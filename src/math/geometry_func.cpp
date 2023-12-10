#include "geometry_func.hpp"
#include "math/math_func.hpp"
namespace epi {
float calcTriangleVolume(vec2f a, vec2f b, vec2f c) {
    auto la = length(a - b);
    auto lb = length(b - c);
    auto lc = length(c - a);
    auto S = la + lb + lc;
    S *= 0.5f;
    return sqrt(S * (S - la) * (S - lb) * (S -lc));
}
#define SQR(x) ((x) * (x))
bool isOverlappingPointAABB(const vec2f& p, const AABB& r) {
    return (p.x >= r.center().x - r.size().x / 2 && p.y > r.center().y - r.size().y / 2
        && p.x < r.center().x + r.size().x / 2 && p.y <= r.center().y + r.size().y / 2);
}
bool isOverlappingPointCircle(const vec2f& p, const Circle& c) {
    return length(p - c.pos) <= c.radius;
}
bool isOverlappingPointPoly(const vec2f& p, const std::vector<vec2f>& points) {
    int i, j, c = 0;
    for (i = 0, j = points.size() - 1; i < points.size(); j = i++) {
        auto& vi = points[i];
        auto& vj = points[j];
        if ( ((vi.y>p.y) != (vj.y>p.y)) &&
             (p.x < (vj.x-vi.x) * (p.y-vi.y) / (vj.y-vi.y) + vi.x) )
               c = !c;
        }
    return c;
}
bool isOverlappingAABBAABB(const AABB& r1, const AABB& r2) {
    return (
        r1.min.x <= r2.max.x &&
        r1.max.x >= r2.min.x &&
        r1.min.y <= r2.max.y &&
        r1.max.y >= r2.min.y);
}
bool AABBcontainsAABB(const AABB& r1, const AABB& r2) {
    return (r2.min.x >= r1.min.x) && (r2.max.x <= r1.max.x) &&
				(r2.min.y >= r1.min.y) && (r2.max.y <= r1.max.y);
}
IntersectionRayAABBResult intersectRayAABB(vec2f ray_origin, vec2f ray_dir,
    const AABB& target)
{
    IntersectionRayAABBResult result;
    vec2f invdir = { 1.0f / ray_dir.x, 1.0f / ray_dir.y };
    vec2f t_size = target.size();
    //VVVVVVVVVVVVV
    //if((int)target.size.y % 2 == 0 && target.pos.y > ray_origin.y)
    //t_size -= vec2f(0, 1);
    //^^^^^^^^^^^^^
    vec2f t_near = (target.center() - t_size / 2.f - ray_origin) * invdir;
    vec2f t_far = (target.center() + t_size / 2.f - ray_origin) * invdir;

    if (std::isnan(t_far.y) || std::isnan(t_far.x)) return {false};
    if (std::isnan(t_near.y) || std::isnan(t_near.x)) return {false};
    if (t_near.x > t_far.x) std::swap(t_near.x, t_far.x);
    if (t_near.y > t_far.y) std::swap(t_near.y, t_far.y);

    if (t_near.x > t_far.y || t_near.y > t_far.x) return {false};
    float t_hit_near = std::max(t_near.x, t_near.y);
    result.time_hit_near = t_hit_near;
    float t_hit_far = std::min(t_far.x, t_far.y);
    result.time_hit_far = t_hit_far;

    if (t_hit_far < 0)
        return {false};
    result.contact_point = ray_origin + ray_dir * t_hit_near;
    if (t_near.x > t_near.y) {
        if (invdir.x < 0)
            result.contact_normal = { 1, 0 };
        else
            result.contact_normal = { -1, 0 };
    } else if (t_near.x < t_near.y) {
        if (invdir.y < 0)
            result.contact_normal = { 0, 1 };
        else
            result.contact_normal = { 0, -1 };
    }
    result.detected = true;
    return result;
}
IntersectionRayRayResult intersectRayRay(vec2f ray0_origin, vec2f ray0_dir,
    vec2f ray1_origin, vec2f ray1_dir)
{
    if (ray0_origin == ray1_origin) {
        return {true, false, ray0_origin, 0.f, 0.f};
    }
    auto dx = ray1_origin.x - ray0_origin.x;
    auto dy = ray1_origin.y - ray0_origin.y;
    float det = ray1_dir.x * ray0_dir.y - ray1_dir.y * ray0_dir.x;
    if (det != 0) { // near parallel line will yield noisy results
        float u = (dy * ray1_dir.x - dx * ray1_dir.y) / det;
        float v = (dy * ray0_dir.x - dx * ray0_dir.y) / det;
        return {u >= 0 && v >= 0 && u <= 1.f && v <= 1.f, false, ray0_origin + ray0_dir * u, u, v};
    }
    return {false, true};
}
/*
    bool detected;
    vec2f contact_point;
    float t_hit_near0;
    float t_hit_near1;
*/
IntersectionRayPolygonResult intersectRayPolygon(vec2f ray_origin, vec2f ray_dir, const ConvexPolygon& poly) {
    for(auto v : poly.getVertecies()) {
        auto intersection = intersectRayRay(ray_origin, ray_dir, poly.getPos(), v - poly.getPos());
        if(intersection.detected) {
            auto closest = findClosestPointOnRay(ray_origin, ray_dir, v);
            return {true, normal(closest - v), closest, qlen(v - closest)};
        }
    }
    return {false};
}
vec2f findClosestPointOnRay(vec2f ray_origin, vec2f ray_dir, vec2f point) {
    float ray_dir_len = length(ray_dir);
    vec2f seg_v_unit = ray_dir / ray_dir_len;
    float proj = dot(point - ray_origin, seg_v_unit);
    if (proj <= 0)
        return ray_origin;
    if (proj >= ray_dir_len)
        return ray_origin + ray_dir;
    return seg_v_unit * proj + ray_origin;
}
vec2f findClosestPointOnEdge(vec2f point, const ConvexPolygon& poly) {
    vec2f closest(INFINITY, INFINITY);
    float closest_dist = INFINITY;
    for(size_t i = 0; i < poly.getVertecies().size(); i++) {
        vec2f a = poly.getVertecies()[i];
        vec2f b = poly.getVertecies()[(i + 1) % poly.getVertecies().size()];
        vec2f adir = b - a;
        vec2f t = findClosestPointOnRay(a, adir, point);
        float dist = length(t - point);
        if(dist < closest_dist) {
            closest_dist = dist;
            closest = t;
        }
    }
    return closest;
}
#define VERY_SMALL_AMOUNT 0.001f
bool nearlyEqual(float a, float b) {
    return abs(a - b) < VERY_SMALL_AMOUNT;
}
bool nearlyEqual(vec2f a, vec2f b) {
    return nearlyEqual(a.x, b.x) && nearlyEqual(a.y, b.y);
}
std::vector<vec2f> findContactPointFast(const ConvexPolygon* p0, const ConvexPolygon* p1, vec2f cn) {
    float best = 0.f;
    std::vector<vec2f> contact_point;
            std::swap(p0, p1);
    for(int i = 0; i < 2; i++) {
        if(i == 1) {
            std::swap(p0, p1);
            cn *= -1.f;
        }
        for(auto p : p0->getVertecies()) {
            auto d = dot(normal(p - p0->getPos()), cn);
            if(abs(best - d) < VERY_SMALL_AMOUNT * VERY_SMALL_AMOUNT) {
                best = d;
                contact_point.push_back(p);
            }
            else if(best > d) {
                best = d;
                contact_point = {p};
            }
        }
    }
    return {contact_point};


}

std::vector<vec2f> findContactPoints(const ConvexPolygon& p0, const ConvexPolygon& p1) {
    std::vector<vec2f> result;
    const ConvexPolygon* poly[] = {&p0, &p1};
    struct Seg {
        char polyID;
        float x_pos;
        size_t segID;
        bool isEnding;
        //if isEnding then x_start_pos is the beggining of ray else ray is the struct
        Ray ray;
    };
    std::vector<Seg> all;
    std::vector<Seg> open[2];
    all.reserve(p1.getVertecies().size() * 2 + p0.getVertecies().size() * 2 );
    open[0].reserve(p0.getVertecies().size());
    open[1].reserve(p1.getVertecies().size());
    size_t cur_id = 0;
    for(char i = 0; i < 2; i++) {
        auto prev = poly[i]->getVertecies().back();
        for(auto p : poly[i]->getVertecies()) {
            auto mi = std::min(prev.x, p.x);
            auto mx = std::max(prev.x, p.x);
            all.push_back({i, mi, cur_id, false, Ray::CreatePoints(prev, p)});
            all.push_back({i, mx, cur_id, true, Ray::CreatePoints(prev, p)});
            cur_id += 1;
            prev = p;
        }
    }
    std::sort(all.begin(), all.end(), 
        [&](const Seg& s1, const Seg& s2)->bool {
            return s1.x_pos < s2.x_pos;
        });
    for(auto a : all) {
        if(!a.isEnding) {
            for(auto p : open[!a.polyID]) {
                auto intersection = intersectRayRay(p.ray.pos, p.ray.dir, a.ray.pos, a.ray.dir);
                if(intersection.detected) {
                    result.push_back(intersection.contact_point);
                }
            }
            open[a.polyID].push_back(a);
        }else {
            auto itr = std::find_if(open[a.polyID].begin(), open[a.polyID].end(),
                [&](const Seg& s1) {
                    return s1.segID == a.segID;
            });
            if(itr == open[a.polyID].end()) {
                //std::cerr << "tried to find a line segment that is not present";
                continue;
            }
            open[a.polyID].erase(itr);
        }
    }
    return result;
}
float area(const std::vector<vec2f>& model) {
    double area = 0.0;
    // Calculate value of shoelace formula
    for (int i = 0; i < model.size(); i++) {
      int i1 = (i + 1) % model.size();
      area += (model[i].y + model[i1].y) * (model[i1].x - model[i].x) / 2.0;
    }
    return abs(area / 2.0);
}
IntersectionPolygonPolygonResult intersectPolygonPolygon(const ConvexPolygon &r1, const ConvexPolygon &r2) {
    const ConvexPolygon *poly1 = &r1;
    const ConvexPolygon *poly2 = &r2;

    float overlap = INFINITY;
    vec2f cn;
    
    for (int shape = 0; shape < 2; shape++) {
        if (shape == 1) {
            poly1 = &r2;
            poly2 = &r1;
        }
        for (int a = 0; a < poly1->getVertecies().size(); a++) {
            int b = (a + 1) % poly1->getVertecies().size();
            vec2f axisProj = { -(poly1->getVertecies()[b].y - poly1->getVertecies()[a].y), poly1->getVertecies()[b].x - poly1->getVertecies()[a].x };
            
            // Optional normalisation of projection axis enhances stability slightly
            float d = sqrtf(axisProj.x * axisProj.x + axisProj.y * axisProj.y);
            axisProj = { axisProj.x / d, axisProj.y / d };

            // Work out min and max 1D points for r1
            float min_r1 = INFINITY, max_r1 = -INFINITY;
            for (int p = 0; p < poly1->getVertecies().size(); p++) {
                float q = (poly1->getVertecies()[p].x * axisProj.x + poly1->getVertecies()[p].y * axisProj.y);
                min_r1 = std::min(min_r1, q);
                max_r1 = std::max(max_r1, q);
            }

            // Work out min and max 1D points for r2
            float min_r2 = INFINITY, max_r2 = -INFINITY;
            for (int p = 0; p < poly2->getVertecies().size(); p++) {
                float q = (poly2->getVertecies()[p].x * axisProj.x + poly2->getVertecies()[p].y * axisProj.y);
                min_r2 = std::min(min_r2, q);
                max_r2 = std::max(max_r2, q);
            }

            // Calculate actual overlap along projected axis, and store the minimum
            if(std::min(max_r1, max_r2) - std::max(min_r1, min_r2) < overlap) {
                overlap = std::min(max_r1, max_r2) - std::max(min_r1, min_r2);
                cn = axisProj;
            }

            if (!(max_r2 >= min_r1 && max_r1 >= min_r2))
                return {false};
        }
    }
    //correcting normal
    float d = dot(r2.getPos() - r1.getPos(), cn);
    if(d > 0.f)
        cn *= -1.f;

    return {true, cn, overlap};
}
IntersectionPolygonCircleResult intersectCirclePolygon(const Circle &c, const ConvexPolygon &r) {
    vec2f max_reach = c.pos + normal(r.getPos() - c.pos) * c.radius;

    vec2f cn;
    vec2f closest(INFINITY, INFINITY);
    vec2f prev = r.getVertecies().back();
    for(const auto& p : r.getVertecies()) {
        vec2f tmp = findClosestPointOnRay(prev, p - prev, c.pos);
        if(qlen(closest - c.pos) > qlen(tmp - c.pos)) {
            closest = tmp;
        }
        prev = p;
    }
    bool isOverlappingPoint = isOverlappingPointPoly(c.pos, r.getVertecies());
    bool isOverlapping = qlen(closest - c.pos) <= c.radius * c.radius || isOverlappingPoint;
    if(!isOverlapping) {
        return {false};
    }
    cn = normal(c.pos - closest);
//    //correcting normal
//    if(dot(cn, r.getPos() - closest) > 0.f) {
//        cn *= -1.f;
//    }
    float l = length(c.pos - closest);
    float overlap = c.radius - l;
    if(isOverlappingPoint) {
        overlap = l + c.radius;
        cn *= -1.f;
    }
    return {true, cn, closest, overlap};
}
IntersectionCircleCircleResult intersectCircleCircle(const Circle &c1, const Circle &c2) {
    vec2f dist = c1.pos - c2.pos;
    float dist_len = length(dist);
    if(dist_len > c1.radius + c2.radius) {
        return {false};
    }
    float overlap = c1.radius + c2.radius - dist_len;
    vec2f contact_point =  dist / dist_len * c2.radius + c2.pos;
    return {true, dist / dist_len, contact_point, overlap};
}
float calculateInertia(const std::vector<vec2f>& model, float mass) {
    float area = 0;
    float mmoi = 0;

    int prev = model.size()-1;
    for (int index = 0; index < model.size(); index++) {
        auto a = model[prev];
        auto b = model[index];

        float area_step = abs(cross(a, b))/2.f;
        float mmoi_step = area_step*(dot(a, a)+dot(b, b)+abs(dot(a, b)))/6.f;

        area += area_step;
        mmoi += mmoi_step;

        prev = index;
    }
    
    double density = mass/area;
    mmoi *= density;
    //mmoi -= mass * dot(center, center);
    if(std::isnan(mmoi)) {
        std::cerr << "mmoi calc erreor!";
        mmoi = 0.f;
    }
    return abs(mmoi);
}
} // namespace epi