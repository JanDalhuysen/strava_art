import {distance, lineString, nearestPointOnLine} from '@turf/turf';
import express from 'express';
import {BaseBuilder, buildGPX} from 'gpx-builder';
import multer from 'multer';
import fetch from 'node-fetch';
import polyline from 'polyline';
import sharp from 'sharp';

const {Point} = BaseBuilder.MODELS;

const app = express();
const port = 3000;

app.set('view engine', 'ejs');
app.use(express.static('public'));

// ------------------------------------------------------------------
// Multer: keep single png/jpg upload
// ------------------------------------------------------------------
const upload =
    multer({dest : 'uploads/', fileFilter : (_, file, cb) => cb(null, /image\/(png|jpe?g)/.test(file.mimetype))});

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// 1) Image → ordered point list
// ------------------------------------------------------------------
async function imageToPoints(filePath)
{
    const {data, info} = await sharp(filePath).greyscale().threshold(128).raw().toBuffer({resolveWithObject : true});

    const w = info.width, h = info.height;
    const pts = [];

    // Very naive skeleton: take every dark pixel
    for (let y = 0; y < h; y += 2)
    { // ↓ skip pixels for speed
        for (let x = 0; x < w; x += 2)
        {
            if (data[y * w + x] < 128)
                pts.push({x, y});
        }
    }
    return pts;
}

// ------------------------------------------------------------------
// 2) Map pixels → WGS-84 (linear transform)
// ------------------------------------------------------------------
function geoPlace(pts, centerLat, centerLon, kmSpan = 10)
{
    const kmPerLat = 111.32;
    const kmPerLon = 111.32 * Math.cos(centerLat * Math.PI / 180);
    const spanLat = kmSpan / kmPerLat;
    const spanLon = kmSpan / kmPerLon;

    return pts.map(p => [centerLon + ((p.x / 1000) - 0.5) * spanLon, centerLat + ((p.y / 1000) - 0.5) * spanLat]);
}

// ------------------------------------------------------------------
// 3) Map-matching with OSRM
// ------------------------------------------------------------------
async function matchRoute(coords)
{
    const url = new URL('https://router.project-osrm.org/match/v1/driving/');
    // const url = new URL('https://routing.openstreetmap.de/routed-bike/route/v1/driving/');
    url.pathname += coords.map(c => c.join(',')).join(';');
    url.searchParams.set('geometries', 'geojson');
    url.searchParams.set('overview', 'full');

    console.log(coords);
    console.log(url.toString());
    console.log('Fetching match from OSRM...');

    const response = await fetch(url);
    if (!response.ok)
    {
        throw new Error(`HTTP error! status: ${response.status}`);
    }
    const result = await response.json();
    console.log('OSRM response:', result);

    if (result.code !== 'Ok' || !result.matchings || !result.matchings.length)
    {
        throw new Error('No match found');
    }

    const match = result.matchings[0];
    return {geometry : match.geometry, distance : match.distance, duration : match.duration};
}

// ------------------------------------------------------------------
// 4) Simple score
// ------------------------------------------------------------------
function scoreRoute(rawCoords, matchedLine)
{
    let totalErr = 0;
    const line = lineString(matchedLine.coordinates);
    rawCoords.forEach(c => { totalErr += distance(c, nearestPointOnLine(line, c)); });
    return {error : totalErr / rawCoords.length, length : matchedLine.properties.distance};
}

// ------------------------------------------------------------------
// 5) GPX export
// ------------------------------------------------------------------
function toGPX(coordinates)
{
    const gpx = buildGPX();
    const trkseg = coordinates.map(c => new Point(c[1], c[0]));
    gpx.tracks = [ {segments : [ trkseg ]} ];
    return gpx.toString();
}

// ------------------------------------------------------------------
// Express routes
// ------------------------------------------------------------------
app.post('/generate', upload.single('image'), async (req, res) => {
    try
    {
        const {lat, lon} = req.body;
        if (!lat || !lon)
            return res.status(400).send('lat & lon required');

        const pts = await imageToPoints(req.file.path);
        const wgs = geoPlace(pts, +lat, +lon);
        const match = await matchRoute(wgs);
        const score = scoreRoute(wgs, match.geometry);

        console.log(pts);
        console.log(wgs);
        console.log(match);
        console.log(score);

        const gpx = toGPX(match.geometry.coordinates);

        console.log('GPX generated');
        console.log(gpx);

        res.set('Content-Type', 'application/gpx+xml');
        res.attachment('strava-art.gpx');
        res.send(gpx);
    }
    catch (e)
    {
        res.status(500).send(String(e));
    }
});

// Health-check
app.get('/', (_, res) => res.render('index'));

app.listen(port, () => console.log(`Listening on http://localhost:${port}`));
