#include "stdafx.h"
#include "point.h"

void clamp(int* n, int a, int b) {
	*n = std::max(a, std::min(b, *n));
}

Point3_S16::Point3_S16(const Point3& p): x {p.x}, y {p.y}, z {p.z} {}

Point3::Point3(const Point3_S16& p): x {p.x}, y {p.y}, z {p.z} {}

FloatRect::FloatRect() : xa{}, ya{}, xb{}, yb{} {}
FloatRect::FloatRect(double ixa, double iya, double ixb, double iyb) :
	xa{ ixa }, ya{ iya }, xb{ ixb }, yb{ iyb } {}
FloatRect::FloatRect(int ixa, int iya, int ixb, int iyb) :
	xa{ (double)ixa }, ya{ (double)iya }, xb{ (double)ixb }, yb{ (double)iyb } {}

Point3& Point3::operator+=(const Point3& p) {
    return *this = *this + p;
}

Point3_S16& Point3_S16::operator+=(const Point3& p) {
    return *this = {x + p.x, y + p.y, z + p.z};
}

Point3& Point3::operator-=(const Point3& p) {
    return *this = *this - p;
}

Point2 operator+(const Point2& p, const Point2& q) {
    return {p.x + q.x, p.y + q.y};
}

Point3 operator+(const Point3& p, const Point3& q) {
    return {p.x + q.x, p.y + q.y, p.z + q.z};
}

Point3 operator-(const Point3& p, const Point3& q) {
    return {p.x - q.x, p.y - q.y, p.z - q.z};
}

Point3 operator-(const Point3& p) {
    return {-p.x, -p.y, -p.z};
}

Point3::operator glm::vec3() const {
	return glm::vec3(x, y, z);
}

FPoint3 operator+(const Point3& p, const FPoint3& q) {
    return {p.x + q.x, p.y + q.y, p.z + q.z};
}

FPoint3 operator+(const FPoint3& p, const FPoint3& q) {
	return { p.x + q.x, p.y + q.y, p.z + q.z };
}

bool operator==(const Point2& a, const Point2& b) {
    return a.x == b.x && a.y == b.y;
}

bool operator==(const Point3& a, const Point3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool operator!=(const Point3& a, const Point3& b) {
	return !(a == b);
}

Point3 operator*(const int a, const Point3& p) {
    return {a*p.x, a*p.y, a*p.z};
}

FPoint3 operator*(const double a, const FPoint3& p) {
    return {a*p.x, a*p.y, a*p.z};
}

FPoint3::operator glm::vec3() const {
	return glm::vec3(x, y, z);
}

std::ostream& operator<<(std::ostream& os, const Point2& p) {
    return os << "(" <<  p.x << "," << p.y << ")";
}

std::ostream& operator<<(std::ostream& os, const Point3& p) {
    return os << "(" <<  p.x << "," << p.y << "," << p.z << ")";
}

std::size_t Point2Hash::operator()(const Point2& p) const {
    return (p.x << 8) + p.y;
}

std::size_t Point3Hash::operator()(const Point3& p) const {
    return (p.x << 16) + (p.y << 8) + p.z;
}
