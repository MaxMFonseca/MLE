#include "Types3D.h"

#include <numeric>

#include "mle/common/Assert.h"
#include "mle/common/Utils.h"
namespace mle {
std::optional<NearFar> Ray3f::intersect(const AABB& box) const {
    f32 tmin = -inf<f32>();
    f32 tmax = inf<f32>();

    for (int i = 0; i < 3; ++i) {
        f32 inv_d = inv_direction_[i];
        f32 t0 = (box.min()[i] - origin_[i]) * inv_d;
        f32 t1 = (box.max()[i] - origin_[i]) * inv_d;

        if (inv_d < 0.0F) {
            std::swap(t0, t1);
        }

        tmin = glm::max(tmin, t0);
        tmax = glm::min(tmax, t1);

        if (tmax < tmin) {
            return std::nullopt;
        }
    }

    return NearFar{.near = tmin, .far = tmax};
}

std::optional<f32> Plane::intersect(const Ray3f& ray) const {
    const f32 denom = glm::dot(normal_, ray.direction());
    if (feq(denom, 0.0F, 1e-6F)) {
        return std::nullopt;
    }

    const f32 t = glm::dot(origin_ - ray.origin(), normal_) / denom;
    if (t < 0.0F) {
        return std::nullopt;
    }

    return t;
}

std::optional<f32> RectPlane::intersect(const Ray3f& ray) const {
    Plane infinite_plane(center_, normal_);
    auto t = infinite_plane.intersect(ray);
    if (!t) {
        return std::nullopt;
    }

    vec3f hit_pos = ray.pointAt(t.value());

    vec3f to_hit = hit_pos - center_;
    f32 u = glm::dot(to_hit, tangent_);
    f32 v = glm::dot(to_hit, bitangent_);

    if (std::abs(u) > width_ * 0.5F || std::abs(v) > height_ * 0.5F) {
        return std::nullopt;
    }

    return t;
}

void RectPlane::recomputeBasis() {
    vec3f up = (std::abs(normal_.y) < 0.99F) ? vec3f{0, 1, 0} : vec3f{1, 0, 0};
    tangent_ = glm::normalize(glm::cross(up, normal_));
    bitangent_ = glm::normalize(glm::cross(normal_, tangent_));
}

std::array<vec3f, 4> RectPlane::getEdges() const {
    const vec3f half_tangent = tangent_ * (width_ * 0.5F);
    const vec3f half_bitangent = bitangent_ * (height_ * 0.5F);

    const vec3f top_left = center_ - half_tangent + half_bitangent;
    const vec3f top_right = center_ + half_tangent + half_bitangent;
    const vec3f bottom_right = center_ + half_tangent - half_bitangent;
    const vec3f bottom_left = center_ - half_tangent - half_bitangent;

    return {top_left, top_right, bottom_right, bottom_left};
}

RectPlane RectPlane::fromSquareLBCorner(vec3f corner, vec3f normal, f32 size) {
    RectPlane plane;
    plane.normal_ = glm::normalize(normal);
    plane.width_ = size;
    plane.height_ = size;

    plane.recomputeBasis();

    plane.center_ = corner + 0.5F * size * (plane.tangent_ + plane.bitangent_);

    return plane;
}

void Frustum::ABCDPlane::normalize() {
    f32 len = glm::length(vec3f(a, b, c));
    if (len > 0.0F) {
        a /= len;
        b /= len;
        c /= len;
        d /= len;
    }
}

Frustum::Frustum(const mat4f& m) :
    l_(m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0], m[3][3] + m[3][0]),
    r_(m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0], m[3][3] - m[3][0]),
    t_(m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1], m[3][3] - m[3][1]),
    b_(m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1], m[3][3] + m[3][1]),
    n_(m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2], m[3][3] + m[3][2]),
    f_(m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2], m[3][3] - m[3][2]) {
    l_.normalize();
    r_.normalize();
    t_.normalize();
    b_.normalize();
    n_.normalize();
    f_.normalize();
}

f32 Frustum::signedDist(const vec3f& p) const {
    return std::max({l_.signedDist(p), r_.signedDist(p), b_.signedDist(p), t_.signedDist(p), n_.signedDist(p), f_.signedDist(p)});
}

bool Frustum::contains(const vec3f& p) const {
    return l_.signedDist(p) >= 0 && r_.signedDist(p) >= 0 && b_.signedDist(p) >= 0 && t_.signedDist(p) >= 0 && n_.signedDist(p) >= 0 && f_.signedDist(p) >= 0;
}
bool Frustum::contains(const RectPlane& plane) const {
    auto corners = plane.getEdges();
    for (const ABCDPlane& p : {l_, r_, b_, t_, n_, f_}) {
        bool all_outside = true;
        for (const auto& c : corners) {
            if (p.signedDist(c) >= 0) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return false;
        }
    }
    return true;
}

bool Frustum::contains(const Sphere& s) const {
    const vec3f& c = s.center();
    const f32 r = s.radius();

    return std::ranges::all_of(std::array{l_, r_, b_, t_, n_, f_}, [&](const ABCDPlane& p) { return p.signedDist(c) >= -r; });
}

std::array<vec3f, 8> Frustum::getCorners() const {
    return {ABCDPlane::intersect(n_, l_, t_), ABCDPlane::intersect(n_, r_, t_), ABCDPlane::intersect(n_, r_, b_), ABCDPlane::intersect(n_, l_, b_),
            ABCDPlane::intersect(f_, l_, t_), ABCDPlane::intersect(f_, r_, t_), ABCDPlane::intersect(f_, r_, b_), ABCDPlane::intersect(f_, l_, b_)};
}

vec3f Frustum::calcCenter(const std::array<vec3f, 8>& corners) {
    return std::accumulate(corners.begin(), corners.end(), vec3f{0.0F}) / 8.F;  // NOLINT
}

Polygon3D Frustum::intersectWithY0() const {
    auto c = getCorners();

    Polygon3D ret;

    auto try_intersect_edge = [](const vec3f& a, const vec3f& b, Polygon3D& out) {
        if ((a.y > 0.0F && b.y < 0.0F) || (a.y < 0.0F && b.y > 0.0F)) {
            f32 t = a.y / (a.y - b.y);
            out.addVert(a + t * (b - a));
        }
    };

    auto edge = [&](int i, int j) { try_intersect_edge(c.at(i), c.at(j), ret); };
    edge(0, 1);
    edge(1, 2);
    edge(2, 3);
    edge(3, 0);
    edge(4, 5);
    edge(5, 6);
    edge(6, 7);
    edge(7, 4);
    edge(0, 4);
    edge(1, 5);
    edge(2, 6);
    edge(3, 7);

    if (ret.vertCount() < 3) {
        return {};
    }

    ret.sortCCW();
    return ret;
}

vec3f Frustum::ABCDPlane::intersect(const Frustum::ABCDPlane& p1, const Frustum::ABCDPlane& p2, const Frustum::ABCDPlane& p3) {
    vec3f n1{p1.a, p1.b, p1.c};
    vec3f n2{p2.a, p2.b, p2.c};
    vec3f n3{p3.a, p3.b, p3.c};

    f32 det = dot(n1, cross(n2, n3));
    MLE_ASSERT_LOG(std::abs(det) > 1e-6F, "Planes do not intersect at a point");

    vec3f c1 = cross(n2, n3) * -p1.d;
    vec3f c2 = cross(n3, n1) * -p2.d;
    vec3f c3 = cross(n1, n2) * -p3.d;

    return (c1 + c2 + c3) / det;
}

Polygon3D::Polygon3D(std::vector<vec3f> verts) :
    verts_(std::move(verts)) {
}

vec3f Polygon3D::center() const {
    vec3f sum{0.0F};
    for (const auto& v : verts_) {
        sum += v;
    }
    return verts_.empty() ? vec3f{0.0F} : sum / static_cast<f32>(verts_.size());
}

vec3f Polygon3D::normal() const {
    if (verts_.size() < 3) {
        return vec3f{0.0F};
    }

    vec3f ret{0.0F};
    for (usize i = 0; i < verts_.size(); ++i) {
        const vec3f& cur = verts_[i];
        const vec3f& next = verts_[(i + 1) % verts_.size()];
        ret.x += (cur.y - next.y) * (cur.z + next.z);
        ret.y += (cur.z - next.z) * (cur.x + next.x);
        ret.z += (cur.x - next.x) * (cur.y + next.y);
    }

    return normalize(ret);
}

void Polygon3D::sortCCW() {
    if (verts_.size() < 3) {
        return;
    }

    vec3f c = center();
    vec3f n = normal();

    vec3f u = normalize(anyPerpendicular(n));
    vec3f v = normalize(cross(n, u));

    std::ranges::sort(verts_, [&](const vec3f& a, const vec3f& b) {
        vec2f da(dot(a - c, u), dot(a - c, v));
        vec2f db(dot(b - c, u), dot(b - c, v));
        return std::atan2(da.y, da.x) < std::atan2(db.y, db.x);
    });
}

vec3f Polygon3D::anyPerpendicular(vec3f v) {
    if (std::abs(v.x) < 0.9F) {
        return normalize(cross(v, vec3f(1, 0, 0)));
    }
    return normalize(cross(v, vec3f(0, 1, 0)));
}

std::vector<vec2f> Polygon3D::xz() {
    std::vector<vec2f> ret;
    ret.reserve(verts_.size());
    for (const auto& v : verts_) {
        ret.emplace_back(v.x, v.z);
    }
    return ret;
}

std::array<vec3f, 8> AABB::corners() const {
    return {{
        {min_.x, min_.y, min_.z},
        {max_.x, min_.y, min_.z},
        {min_.x, max_.y, min_.z},
        {max_.x, max_.y, min_.z},
        {min_.x, min_.y, max_.z},
        {max_.x, min_.y, max_.z},
        {min_.x, max_.y, max_.z},
        {max_.x, max_.y, max_.z},
    }};
}

bool Ray3f::intersects(const Sphere& sphere) const {
    vec3f oc = origin_ - sphere.center();
    f32 b = glm::dot(oc, direction_);
    f32 c = glm::dot(oc, oc) - (sphere.radius() * sphere.radius());
    f32 disc = (b * b) - c;
    return disc >= 0.0F;
}

bool AABB::intersects(const Sphere& sphere) const {
    vec3f closest = glm::clamp(sphere.center(), min_, max_);
    f32 dist_sq = glm::distance2(closest, sphere.center());
    return dist_sq <= sphere.radius() * sphere.radius();
}

bool Frustum::contains(const AABB& box) const {
    const auto corners = box.corners();

    for (const auto& plane : {l_, r_, b_, t_, n_, f_}) {
        bool all_outside = true;
        for (const auto& corner : corners) {
            if (plane.signedDist(corner) >= 0.0F) {
                all_outside = false;
                break;
            }
        }
        if (all_outside) {
            return false;
        }
    }

    return true;
}
}  // namespace mle
