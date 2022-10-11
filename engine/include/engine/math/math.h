#pragma once

#include "engine/base/panic.h"

#include <cmath>

namespace engine::math {
    template<typename T>
    struct Vec3;

    template<typename T>
    struct Vec4;

    template<typename T>
    struct Resolution {
        T width;
        T height;

        constexpr static Resolution of(T it) {
            return { it, it };
        }

        template<typename U>
        constexpr U aspectRatio() const {
            return U(width) / U(height);
        }
    };

    template<typename T>
    struct Vec2 {
        T x;
        T y;

        constexpr bool operator==(T other) const {
            return equal(of(other));
        }

        constexpr bool operator==(const Vec2& other) const {
            return equal(other);
        }

        constexpr bool equal(const Vec2 &other) const {
            return x == other.x && y == other.y;
        }

        constexpr Vec2 operator+(const Vec2& other) const {
            return { x + other.x, y + other.y };
        }

        constexpr Vec2 operator*(T it) const {
            return from(x * it, y * it);
        }

        static constexpr Vec2 from(T x, T y) {
            return { x, y };
        }

        static constexpr Vec2 of(T it) {
            return from(it, it);
        }

        constexpr Vec3<T> vec3(T z) const {
            return Vec3<T>::from(x, y, z);
        }
    };

    template<typename T>
    struct Vec3 {
        T x;
        T y;
        T z;

        static constexpr Vec3 from(T x, T y, T z) {
            return { x, y, z };
        }

        static constexpr Vec3 of(T it) {
            return from(it, it, it);
        }

        constexpr Vec3 operator-(const Vec3& it) const {
            return from(x - it.x, y - it.y, z - it.z);
        }

        constexpr bool operator!=(const Vec3& it) const {
            return x != it.x || y != it.y || z != it.z;
        }

        bool isinf() const {
            return std::isinf(x) || std::isinf(y) || std::isinf(z);
        }

        static constexpr Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
            return from(
                lhs.y * rhs.z - lhs.z * rhs.y, 
                lhs.z * rhs.x - lhs.x * rhs.z, 
                lhs.x * rhs.y - lhs.y * rhs.x
            );
        }

        static constexpr T dot(const Vec3& lhs, const Vec3& rhs) {
            return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
        }

        constexpr T length() const {
            return std::sqrt(x * x + y * y + z * z);
        }

        constexpr Vec3 normal() const {
            auto len = length();
            return from(x / len, y / len, z / len);
        }

        constexpr Vec3 negate() const {
            return from(-x, -y, -z);
        }

        constexpr Vec4<T> vec4(T w) const {
            return Vec4<T>::from(x, y, z, w);
        }

        constexpr Vec3 operator*(T it) const {
            return from(x * it, y * it, z * it);
        }
    };

    template<typename T>
    struct Vec4 {
        enum Select { X = (1 << 0), Y = (1 << 1), Z = (1 << 2), W = (1 << 3) };

        T x;
        T y;
        T z;
        T w;

        static constexpr Vec4 from(T x, T y, T z, T w) {
            return { x, y, z, w };
        }

        static constexpr Vec4 of(T it) {
            return from(it, it, it, it);
        }

        static constexpr Vec4 select(const Vec4& lhs, const Vec4& rhs, Select control) {
            auto x = control & X ? rhs.x : lhs.x;
            auto y = control & Y ? rhs.y : lhs.y;
            auto z = control & Z ? rhs.z : lhs.z;
            auto w = control & W ? rhs.w : lhs.w;

            return from(x, y, z, w);
        }

        constexpr T length() const {
            return std::sqrt(x * x + y * y + z * z + w * w);
        }

        constexpr Vec4 negate() const {
            return from(-x, -y, -z, -w);
        }

        constexpr Vec4 normal() const {
            auto len = length();
            return from(x / len, y / len, z / len, w / len);
        }

        constexpr Vec3<T> vec3() const {
            return Vec3<T>::from(x, y, z);
        }

        constexpr const T& at(size_t index) const { 
            return this->*components[index];
        }

    private:
        static constexpr T Vec4::*components[] { &Vec4::x, &Vec4::y, &Vec4::z, &Vec4::w };
    };

    template<typename T>
    struct Mat3x3 {
        Vec3<T> rows[3];

        static constexpr Mat3x3 from(const Vec3<T>& row0, const Vec3<T>& row1, const Vec3<T>& row2) {
            return { row0, row1, row2 };
        }

        static constexpr Mat3x3 of(T it) {
            return from(Vec3<T>::of(it), Vec3<T>::of(it), Vec3<T>::of(it));
        }

        static constexpr Mat3x3 identity() {
            auto row1 = Vec3<T>::from(1, 0, 0);
            auto row2 = Vec3<T>::from(0, 1, 0);
            auto row3 = Vec3<T>::from(0, 0, 1);
            return from(row1, row2, row3);
        }
    };

    template<typename T>
    struct Mat4x4 {
        using Row = Vec4<T>;
        using Row3 = Vec3<T>;
        using RowSelect = typename Row::Select;
        Row rows[4];

        constexpr const Row &at(size_t row) const {
            return rows[row];
        }

        constexpr Row& operator[](size_t row) const {
            return rows[row];
        }

        constexpr T at(size_t row, size_t col) const {
            return at(row).at(col);
        }

        constexpr Row mul(Row other) const {
            auto row0 = at(0);
            auto row1 = at(1);
            auto row2 = at(2);
            auto row3 = at(3);

            auto x = row0.x * other.x + row0.y * other.y + row0.z * other.z + row0.w * other.w;
            auto y = row1.x * other.x + row1.y * other.y + row1.z * other.z + row1.w * other.w;
            auto z = row2.x * other.x + row2.y * other.y + row2.z * other.z + row2.w * other.w;
            auto w = row3.x * other.x + row3.y * other.y + row3.z * other.z + row3.w * other.w;

            return Row::from(x, y, z, w);
        }

        constexpr Mat4x4 mul(const Mat4x4& other) const {
            auto row0 = at(0);
            auto row1 = at(1);
            auto row2 = at(2);
            auto row3 = at(3);

            auto other0 = other.at(0);
            auto other1 = other.at(1);
            auto other2 = other.at(2);
            auto other3 = other.at(3);

            auto out0 = Row::from(
                (other0.x * row0.x) + (other1.x * row0.y) + (other2.x * row0.z) + (other3.x * row0.w),
                (other0.y * row0.x) + (other1.y * row0.y) + (other2.y * row0.z) + (other3.y * row0.w),
                (other0.z * row0.x) + (other1.z * row0.y) + (other2.z * row0.z) + (other3.z * row0.w),
                (other0.w * row0.x) + (other1.w * row0.y) + (other2.w * row0.z) + (other3.w * row0.w)
            );

            auto out1 = Row::from(
                (other0.x * row1.x) + (other1.x * row1.y) + (other2.x * row1.z) + (other3.x * row1.w),
                (other0.y * row1.x) + (other1.y * row1.y) + (other2.y * row1.z) + (other3.y * row1.w),
                (other0.z * row1.x) + (other1.z * row1.y) + (other2.z * row1.z) + (other3.z * row1.w),
                (other0.w * row1.x) + (other1.w * row1.y) + (other2.w * row1.z) + (other3.w * row1.w)
            );

            auto out2 = Row::from(
                (other0.x * row2.x) + (other1.x * row2.y) + (other2.x * row2.z) + (other3.x * row2.w),
                (other0.y * row2.x) + (other1.y * row2.y) + (other2.y * row2.z) + (other3.y * row2.w),
                (other0.z * row2.x) + (other1.z * row2.y) + (other2.z * row2.z) + (other3.z * row2.w),
                (other0.w * row2.x) + (other1.w * row2.y) + (other2.w * row2.z) + (other3.w * row2.w)
            );
            
            auto out3 = Row::from(
                (other0.x * row3.x) + (other1.x * row3.y) + (other2.x * row3.z) + (other3.x * row3.w),
                (other0.y * row3.x) + (other1.y * row3.y) + (other2.y * row3.z) + (other3.y * row3.w),
                (other0.z * row3.x) + (other1.z * row3.y) + (other2.z * row3.z) + (other3.z * row3.w),
                (other0.w * row3.x) + (other1.w * row3.y) + (other2.w * row3.z) + (other3.w * row3.w)
            );
            
            return Mat4x4::from(out0, out1, out2, out3);
        }

        constexpr Mat4x4 operator*(const Mat4x4& other) const {
            return mul(other);
        }

        constexpr Mat4x4 operator*=(const Mat4x4& other) {
            return *this = *this * other;
        }

        static constexpr Mat4x4 from(const Row& row0, const Row& row1, const Row& row2, const Row& row3) {
            return { { row0, row1, row2, row3 } };
        }

        static constexpr Mat4x4 of(T it) {
            return from(Row::of(it), Row::of(it), Row::of(it), Row::of(it));
        }

        static constexpr Mat4x4 scaling(T x, T y, T z) {
            auto row0 = Row::from(x, 0, 0, 0);
            auto row1 = Row::from(0, y, 0, 0);
            auto row2 = Row::from(0, 0, z, 0);
            auto row3 = Row::from(0, 0, 0, 1);
            return from(row0, row1, row2, row3);
        }

        static constexpr Mat4x4 translation(T x, T y, T z) {
            auto row0 = Row::from(1, 0, 0, x);
            auto row1 = Row::from(0, 1, 0, y);
            auto row2 = Row::from(0, 0, 1, z);
            auto row3 = Row::from(0, 0, 0, 1);
            return from(row0, row1, row2, row3);
        }

        static constexpr Mat4x4 rotation(T angle, Vec3<T> axis) {
            auto c = std::cos(angle);
            auto s = std::sin(angle);
            auto t = 1 - c;

            auto x = axis.x;
            auto y = axis.y;
            auto z = axis.z;

            auto xy = x * y;
            auto xz = x * z;
            auto yz = y * z;

            auto xs = x * s;
            auto ys = y * s;
            auto zs = z * s;

            auto row0 = Row::from(
                t * x * x + c,
                xy * t + zs,
                xz * t - ys,
                0
            );

            auto row1 = Row::from(
                xy * t - zs,
                t * y * y + c,
                yz * t + xs,
                0
            );

            auto row2 = Row::from(
                xz * t + ys,
                yz * t - xs,
                t * z * z + c,
                0
            );

            auto row3 = Row::from(
                0,
                0,
                0,
                1
            );

            return from(row0, row1, row2, row3);
        }

        constexpr Mat4x4 transpose() const {
            auto r0 = Row::from(rows[0].x, rows[1].x, rows[2].x, rows[3].x);
            auto r1 = Row::from(rows[0].y, rows[1].y, rows[2].y, rows[3].y);
            auto r2 = Row::from(rows[0].z, rows[1].z, rows[2].z, rows[3].z);
            auto r3 = Row::from(rows[0].w, rows[1].w, rows[2].w, rows[3].w);
            return from(r0, r1, r2, r3);
        }

        static constexpr Mat4x4 identity() {
            auto row0 = Row::from(1, 0, 0, 0);
            auto row1 = Row::from(0, 1, 0, 0);
            auto row2 = Row::from(0, 0, 1, 0);
            auto row3 = Row::from(0, 0, 0, 1);
            return from(row0, row1, row2, row3);
        }

        static constexpr Mat4x4 lookToLH(const Row3& eye, const Row3& dir, const Row3& up) {
            ASSERT(eye != Row3::of(0));
            ASSERT(up != Row3::of(0));

            ASSERT(!eye.isinf());
            ASSERT(!up.isinf());

            auto r2 = dir.normal();
            auto r0 = Row3::cross(up, r2).normal();
            auto r1 = Row3::cross(r2, r0);

            auto negEye = eye.negate();

            auto d0 = Row3::dot(r0, negEye);
            auto d1 = Row3::dot(r1, negEye);
            auto d2 = Row3::dot(r2, negEye);

            auto s0 = r0.vec4(d0);
            auto s1 = r1.vec4(d1);
            auto s2 = r2.vec4(d2);
            auto s3 = Row::from(0, 0, 0, 1);

            return from(s0, s1, s2, s3).transpose();
        }

        static constexpr Mat4x4 lookToRH(const Row3& eye, const Row3& dir, const Row3& up) {
            return lookToLH(eye, dir.negate(), up);
        }

        static constexpr Mat4x4 lookAtRH(const Row3& eye, const Row3& focus, const Row3& up) {
            return lookToLH(eye, eye - focus, up);
        }

        static constexpr Mat4x4 perspectiveRH(T fov, T aspect, T nearLimit, T farLimit) {
            auto fovSin = std::sin(fov / 2);
            auto fovCos = std::cos(fov / 2);

            auto height = fovCos / fovSin;
            auto width = height / aspect;
            auto range = farLimit / (nearLimit - farLimit);

            auto r0 = Row::from(width, 0, 0, 0);
            auto r1 = Row::from(0, height, 0, 0);
            auto r2 = Row::from(0, 0, range, -1);
            auto r3 = Row::from(0, 0, range * nearLimit, 0);
            return from(r0, r1, r2, r3);
        }
    };

    using float2 = Vec2<float>;
    using float3 = Vec3<float>;
    using float4 = Vec4<float>;
    using float3x3 = Mat3x3<float>;
    using float4x4 = Mat4x4<float>;

    static_assert(sizeof(float2) == sizeof(float) * 2);
    static_assert(sizeof(float3) == sizeof(float) * 3);
    static_assert(sizeof(float4) == sizeof(float) * 4);
    static_assert(sizeof(float3x3) == sizeof(float) * 3 * 3);
    static_assert(sizeof(float4x4) == sizeof(float) * 4 * 4);
}
