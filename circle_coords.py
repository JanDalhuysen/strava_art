def generate_circle_coordinates(center_x, center_y, radius):
    """
    Generates coordinates for plotting a circle on grid paper using the Midpoint Circle Algorithm.

    Args:
        center_x (int): The x-coordinate of the circle's center.
        center_y (int): The y-coordinate of the circle's center.
        radius (int): The radius of the circle.

    Returns:
        list of tuples: A list of (x, y) coordinates representing the circle's perimeter.
    """
    x = radius
    y = 0
    err = 0
    coordinates = set() # Use a set to avoid duplicate points

    while x >= y:
        # These 8 lines plot the points in all eight octants
        coordinates.add((center_x + x, center_y + y))
        coordinates.add((center_x + y, center_y + x))
        coordinates.add((center_x - y, center_y + x))
        coordinates.add((center_x - x, center_y + y))
        coordinates.add((center_x - x, center_y - y))
        coordinates.add((center_x - y, center_y - x))
        coordinates.add((center_x + y, center_y - x))
        coordinates.add((center_x + x, center_y - y))

        if err <= 0:
            y += 1
            err += 2 * y + 1
        if err > 0:
            x -= 1
            err -= 2 * x + 1
    
    # Sort for easier plotting
    return sorted(list(coordinates))

# --- Example Usage ---
center_x = 1
center_y = 1
radius = 1

coords = generate_circle_coordinates(center_x, center_y, radius)

print(f"Coordinates to plot a circle centered at ({center_x}, {center_y}) with radius {radius}:")
for point in coords:
    print(point)
