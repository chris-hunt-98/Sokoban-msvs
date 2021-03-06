#ifndef POINT_H
#define POINT_H

void clamp(int* n, int a, int b);

// Standard Points serialize their components as unsigned chars
struct Point2 {
    int x;
    int y;
};

struct Point3;

//Point3_S16 serializes its components as signed 2-byte integers
struct Point3_S16 {
    int x;
    int y;
    int z;
    Point3_S16(): x {0}, y {0}, z {0} {}
    Point3_S16(int ax, int ay, int az): x {ax}, y {ay}, z {az} {}
    Point3_S16& operator+=(const Point3& p);
    explicit Point3_S16(const Point3&);
};

struct Point3 {
    int x;
    int y;
    int z;
    Point3& operator+=(const Point3& p);
    Point3& operator-=(const Point3& p);
    Point2 h() {return {x,y};}
    Point3(): x {0}, y {0}, z {0} {}
    Point3(int ax, int ay, int az): x {ax}, y {ay}, z {az} {}
    explicit Point3(const Point3_S16&);
	explicit operator glm::vec3() const;
};

struct FPoint3 {
    double x;
    double y;
    double z;
    FPoint3(): x {}, y {}, z {} {}
    FPoint3(double ax, double ay, double az): x {ax}, y {ay}, z {az} {}
    FPoint3(const Point3& p): x {static_cast<double>(p.x)}, y {static_cast<double>(p.y)}, z {static_cast<double>(p.z)} {}
	explicit operator glm::vec3() const;
};

struct IntRect {
	int xa, ya, xb, yb;
};

struct FloatRect {
	double xa, ya, xb, yb;
	FloatRect();
	FloatRect(double xa, double ya, double xb, double yb);
	FloatRect(int xa, int ya, int xb, int yb);
};

Point2 operator+(const Point2& p, const Point2& q);

Point3 operator+(const Point3& p, const Point3& q);
Point3 operator-(const Point3& p, const Point3& q);

Point3 operator-(const Point3& p);

FPoint3 operator+(const Point3& p, const FPoint3& q);
FPoint3 operator+(const FPoint3& p, const FPoint3& q);


bool operator==(const Point2& a, const Point2& b);

bool operator==(const Point3& a, const Point3& b);
bool operator!=(const Point3& a, const Point3& b);

Point3 operator*(const int a, const Point3& p);

FPoint3 operator*(const double a, const FPoint3& p);

std::ostream& operator<<(std::ostream& os, const Point2& p);

std::ostream& operator<<(std::ostream& os, const Point3& p);

struct Point2Hash {
    std::size_t operator()(const Point2& p) const;
};

struct Point3Hash {
    std::size_t operator()(const Point3& p) const;
};

#endif // POINT_H
