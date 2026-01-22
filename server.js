import { distance, lineString, nearestPointOnLine } from "@turf/turf";
import express from "express";
import { BaseBuilder, buildGPX, GarminBuilder } from "gpx-builder";
import multer from "multer";
import fetch from "node-fetch";
import polyline from "polyline";
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
// ------------------------------------------------------------------
// 1) Image → ordered point list
// ------------------------------------------------------------------
async function imageToPoints(filePath) {
  const { data, info } = await sharp(filePath).greyscale().threshold(128).raw().toBuffer({ resolveWithObject: true });

  const w = info.width,
    h = info.height;
  const pts = [];

  // Collect all dark pixels with finer sampling
  for (let y = 0; y < h; y += 2) {
    for (let x = 0; x < w; x += 2) {
      if (data[y * w + x] < 128) pts.push({ x, y });
    }
  }

  if (pts.length < 2) {
    throw new Error("Image has too few dark pixels. Try a darker or more detailed image.");
  }

  // Order points using nearest-neighbor to form a continuous path
  const ordered = [pts[0]];
  const remaining = new Set(pts.slice(1));

  while (remaining.size > 0) {
    const last = ordered[ordered.length - 1];
    let nearest = null;
    let minDist = Infinity;

    for (const pt of remaining) {
      const dist = Math.hypot(pt.x - last.x, pt.y - last.y);
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

  return ordered;
}

// ------------------------------------------------------------------
// 2) Map pixels → WGS-84 (linear transform)
// ------------------------------------------------------------------
function geoPlace(pts, centerLat, centerLon, kmSpan = 10) {
  const kmPerLat = 1.32;
  const kmPerLon = 1.32 * Math.cos((centerLat * Math.PI) / 180);
  const spanLat = kmSpan / kmPerLat;
  const spanLon = kmSpan / kmPerLon;

  return pts.map((p) => [centerLon + (p.x / 1000 - 0.5) * spanLon, centerLat + (p.y / 1000 - 0.5) * spanLat]);
}

// ------------------------------------------------------------------
// 3) Map-matching with OSRM
// ------------------------------------------------------------------
async function matchRoute(coords) {
  const limitedCoords = coords.slice(0, 40);
  const url = new URL("https://router.project-osrm.org/match/v1/driving/");
  url.pathname += limitedCoords.map((c) => c.join(",")).join(";");
  url.searchParams.set("geometries", "geojson");
  url.searchParams.set("overview", "full");

  console.log(limitedCoords);
  // console.log(url.toString());
  // console.log("Fetching match from OSRM...");

  // const response = await fetch(url);
  // if (!response.ok) {
  //   throw new Error(`HTTP error! status: ${response.status}`);
  // }
  // const result = await response.json();
  // console.log("OSRM response:", result);

  // if (result.code !== "Ok" || !result.matchings || !result.matchings.length) {
  //   throw new Error("No match found");
  // }

  // const match = result.matchings[0];
  // return {
  //   geometry: match.geometry,
  //   distance: match.distance,
  //   duration: match.duration,
  // };
}

// ------------------------------------------------------------------
// 4) Simple score
// ------------------------------------------------------------------
function scoreRoute(rawCoords, match) {
  let totalErr = 0;
  const line = lineString(match.geometry.coordinates);
  rawCoords.forEach((c) => {
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

    const pts = await imageToPoints(req.file.path);
    const wgs = geoPlace(pts, +lat, +lon);
    const match = await matchRoute(wgs);
    const score = scoreRoute(wgs, match);

    console.log(pts);
    console.log(wgs);
    console.log(match);
    console.log(score);

    const gpx = toGPX(match.geometry.coordinates);

    console.log("GPX generated");
    console.log(gpx);

    res.set("Content-Type", "application/gpx+xml");
    res.attachment("strava-art.gpx");
    res.send(gpx);
  } catch (e) {
    res.status(500).send(String(e));
  }
});

// Health-check
app.get("/", (_, res) => res.render("index"));

app.listen(port, () => console.log(`Listening on http://localhost:${port}`));
