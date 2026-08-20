#pragma once
#include <string>
#include <algorithm>
#include <cmath>

// ── Basic record type used by all 1-D indices ──────────────────────────
struct Record {
    int    key;
    std::string value;
};

// ── 2-D point for R-Tree ───────────────────────────────────────────────
struct Point2D {
    double x = 0.0;
    double y = 0.0;
    bool operator==(const Point2D& o) const {
        return std::fabs(x - o.x) < 1e-9 && std::fabs(y - o.y) < 1e-9;
    }
};

// ── Minimum Bounding Rectangle for R-Tree ──────────────────────────────
struct MBR {
    double x_min = 0, y_min = 0, x_max = 0, y_max = 0;

    double area() const {
        return (x_max - x_min) * (y_max - y_min);
    }

    bool contains(const Point2D& p) const {
        return p.x >= x_min && p.x <= x_max &&
               p.y >= y_min && p.y <= y_max;
    }

    bool intersects(const MBR& o) const {
        return !(o.x_min > x_max || o.x_max < x_min ||
                 o.y_min > y_max || o.y_max < y_min);
    }

    // Enlarge to include a point
    void expand(const Point2D& p) {
        x_min = std::min(x_min, p.x);
        y_min = std::min(y_min, p.y);
        x_max = std::max(x_max, p.x);
        y_max = std::max(y_max, p.y);
    }

    // Enlarge to include another MBR
    void expand(const MBR& o) {
        x_min = std::min(x_min, o.x_min);
        y_min = std::min(y_min, o.y_min);
        x_max = std::max(x_max, o.x_max);
        y_max = std::max(y_max, o.y_max);
    }

    // Area increase needed to include another MBR
    double enlargement(const MBR& o) const {
        double nx_min = std::min(x_min, o.x_min);
        double ny_min = std::min(y_min, o.y_min);
        double nx_max = std::max(x_max, o.x_max);
        double ny_max = std::max(y_max, o.y_max);
        return (nx_max - nx_min) * (ny_max - ny_min) - area();
    }

    static MBR from_point(const Point2D& p) {
        return {p.x, p.y, p.x, p.y};
    }
};
