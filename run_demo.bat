@echo off
REM Complete demo: generate grid, circle, match, and visualize

echo.
echo ========================================
echo Strava Art - Circle to Grid Matching
echo ========================================
echo.

echo Step 1: Generating 20 points on a circle...
gen_circle 10
echo.

echo Step 2: Generating 6x6 Manhattan grid...
gen_manhattan_grid 9 9 0.5
echo.


echo Step 3: Matching circle points to grid...
tiny_mm
echo.

echo Step 4: Creating visualization...
visualise
echo.

echo ========================================
echo Done! Open map.svg in a web browser
echo ========================================
