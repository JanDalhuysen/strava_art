import { distance, lineString, nearestPointOnLine } from "@turf/turf";
import express from "express";
import { BaseBuilder, buildGPX, GarminBuilder } from "gpx-builder";
import multer from "multer";
import fetch from "node-fetch";
import sharp from "sharp";

const { Point } = BaseBuilder.MODELS;

const app = express();
const port = 3000;

app.set("view engine", "ejs");
app.use(express.static("public"));

// ------------------------------------------------------------------
// Multer: keep single png/jpg upload
// ------------------------------------------------------------------
const upload = multer({
  dest: "uploads/",
  fileFilter: (_, file, cb) => cb(null, /image\/(png|jpe?g)/.test(file.mimetype)),
});

// ------------------------------------------------------------------
// 1) Image → ordered point list
// ------------------------------------------------------------------
async function imageToPoints(filePath) {
  // Resize to a manageable size (e.g., 150px width) to reduce noise and point count
  // This also effectively performs a "downsampling" which helps with smoothing
  const pipeline = sharp(filePath).resize(150).greyscale().threshold(128).raw();

  const { data, info } = await pipeline.toBuffer({ resolveWithObject: true });

  const w = info.width,
    h = info.height;
  const pts = [];

  // Collect all dark pixels
  for (let y = 0; y < h; y++) {
    for (let x = 0; x < w; x++) {
      if (data[y * w + x] < 128) pts.push({ x, y });
    }
  }

  if (pts.length < 2) {
    throw new Error("Image has too few dark pixels. Try a darker or more detailed image.");
  }

  // Order points using nearest-neighbor to form a continuous path
  // Start from the first point found
  const ordered = [pts[0]];
  const remaining = new Set(pts.slice(1));

  // Optimization: Spatial hashing could speed this up, but for 150px image, N is small enough
  while (remaining.size > 0) {
    const last = ordered[ordered.length - 1];
    let nearest = null;
    let minDist = Infinity;

    // Simple greedy search
    for (const pt of remaining) {
      const dist = (pt.x - last.x) ** 2 + (pt.y - last.y) ** 2; // Squared distance is enough for comparison
      if (dist < minDist) {
        minDist = dist;
        nearest = pt;
      }
    }

    if (nearest) {
      ordered.push(nearest);
      remaining.delete(nearest);
    } else {
      break;
    }
  }

  return { points: ordered, width: w, height: h };
}

// ------------------------------------------------------------------
// 2) Map pixels → WGS-84 (linear transform)
// ------------------------------------------------------------------
function geoPlace(imgData, centerLat, centerLon, kmSpan = 3) {
  const { points, width, height } = imgData;

  // 1 degree latitude is approx 111.32 km
  const kmPerLat = 111.32;
  // 1 degree longitude depends on latitude
  const kmPerLon = 111.32 * Math.cos((centerLat * Math.PI) / 180);

  const spanLat = kmSpan / kmPerLat;
  const spanLon = kmSpan / kmPerLon;

  return points.map((p) => [
    centerLon + (p.x / width - 0.5) * spanLon,
    centerLat + (0.5 - p.y / height) * spanLat, // Flip Y because image Y goes down, Lat goes up
  ]);
}

// ------------------------------------------------------------------
// 3) Map-matching with OSRM
// ------------------------------------------------------------------
async function matchRoute(coords) {
  // OSRM public demo server often limits to ~100, but recommendations suggest 25 for safety.
  // We sub-sample to avoid "TooBig" errors.
  const MAX_POINTS = 5;
  const step = Math.ceil(coords.length / MAX_POINTS);
  const limitedCoords = coords
    .filter((_, i) => i % step === 0)
    .slice(0, MAX_POINTS)
    // Round to 5 decimal places to reduce URL length
    .map((c) => [Number(c[0].toFixed(5)), Number(c[1].toFixed(5))]);

  const url = new URL("https://router.project-osrm.org/match/v1/walking/"); // Use walking profile for strava art
  url.pathname += limitedCoords.map((c) => c.join(",")).join(";");
  url.searchParams.set("geometries", "geojson");
  url.searchParams.set("overview", "full");

  // Timestamps can help OSRM but are optional. Omit for now.

  console.log(`Requesting OSRM match for ${limitedCoords.length} points...`);

  try {
    const response = await fetch(url);
    if (!response.ok) {
      const txt = await response.text();
      throw new Error(`OSRM HTTP error ${response.status}: ${txt}`);
    }
    const result = await response.json();

    if (result.code !== "Ok" || !result.matchings || !result.matchings.length) {
      console.warn("OSRM returned no match:", result.code, result.message);
      throw new Error(`OSRM Error: ${result.code} - ${result.message || "No match found"}`);
    }

    // Return the best match (highest confidence)
    const match = result.matchings[0];
    return {
      geometry: match.geometry,
      distance: match.distance,
      duration: match.duration,
    };
  } catch (err) {
    console.error("OSRM call failed:", err);
    throw err;
  }
}

// ------------------------------------------------------------------
// 4) Simple score
// ------------------------------------------------------------------
function scoreRoute(rawCoords, match) {
  let totalErr = 0;
  const line = lineString(match.geometry.coordinates);
  rawCoords.forEach((c) => {
    // Turf distance is in km by default? No, degrees if units not specified?
    // Turf distance default units is kilometers
    totalErr += distance(c, nearestPointOnLine(line, c));
  });
  return {
    error: totalErr / rawCoords.length,
    length: match.distance,
  };
}

// ------------------------------------------------------------------
// 5) GPX export
// ------------------------------------------------------------------
function toGPX(coordinates) {
  const points = coordinates.map((c) => new Point(c[1], c[0]));
  const gb = new GarminBuilder();
  gb.setSegmentPoints(points);
  return buildGPX(gb.toObject());
}

// ------------------------------------------------------------------
// Express routes
// ------------------------------------------------------------------
app.post("/generate", upload.single("image"), async (req, res) => {
  try {
    const { lat, lon } = req.body;
    if (!lat || !lon) return res.status(400).send("lat & lon required");

    console.log("Processing image...");
    const imgData = await imageToPoints(req.file.path);
    console.log(`Extracted ${imgData.points.length} points from image.`);

    console.log("Mapping to coordinates...");
    const wgs = geoPlace(imgData, +lat, +lon);

    console.log("Matching with OSRM...");
    const match = await matchRoute(wgs);

    const score = scoreRoute(wgs, match);
    console.log(`Route matched! Distance: ${match.distance}m, Error Score: ${score.error}`);

    const gpx = toGPX(match.geometry.coordinates);

    res.set("Content-Type", "application/gpx+xml");
    res.attachment("strava-art.gpx");
    res.send(gpx);
  } catch (e) {
    console.error("Generation failed:", e);
    res.status(500).send("Error generating route: " + e.message);
  }
});

// Health-check
app.get("/", (_, res) => res.render("index"));

app.listen(port, () => console.log(`Listening on http://localhost:${port}`));
