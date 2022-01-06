#pragma once

#include <cmath>
#include <DirectXMath.h>

namespace engine::math {
    template<typename T>
    struct Vec3;

    template<typename T>
    struct Vec4;

    template<typename T>
    struct Resolution {
        T width;
        T height;

        float aspectRatio() const {
            return float(width) / float(height);
        }
    };

    template<typename T>
    struct Vec2 {
        T x;
        T y;

        static constexpr Vec2 from(T x, T y) {
            return { x, y };
        }

        static constexpr Vec2 of(T it) {
            return from(it, it);
        }
    };

    template<typename T>
    struct Vec3 {
        T x;
        T y;
        T z;

        static constexpr Vec3 from(T x, T y, T z) {
            return { x, z, y };
        }

        static constexpr Vec3 of(T it) {
            return from(it, it, it);
        }

        static constexpr Vec3 cross(const Vec3& lhs, const Vec3& rhs) {
            return from(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
        }

        static constexpr Vec3 dot(const Vec3& lhs, const Vec3& rhs) {
            return from(lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z);
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

        constexpr Vec4<T> vec4() const {
            return Vec4<T>::from(x, y, z, 0);
        }
    };

    template<typename T>
    struct Vec4 {
        T x;
        T y;
        T z;
        T w;

        static constexpr Vec4 from(T x, T y, T z, T w) {
            return { x, z, y, w };
        }

        static constexpr Vec4 of(T it) {
            return from(it, it, it, it);
        }

        static constexpr Vec4 select(const Vec4& lhs, const Vec4& rhs, const Vec4<bool>& control) {
            auto x = control.x ? rhs.x : lhs.x;
            auto y = control.y ? rhs.y : lhs.y;
            auto z = control.z ? rhs.z : lhs.z;
            auto w = control.w ? rhs.w : lhs.w;

            return from(x, y, z, w);
        }

        static constexpr Vec4 dot(const Vec4& lhs, const Vec4& rhs) {
            auto value = lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z + lhs.w * rhs.w;
            return of(value);
        }

        static constexpr Vec4 cross(const Vec4& lhs, const Vec4& rhs) {
            auto [x, y, z] = Vec3<T>::cross(lhs.vec3(), rhs.vec3());
            return from(x, y, z, 0);
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
        Row rows[4];

        static constexpr Mat4x4 from(const Row& row0, const Row& row1, const Row& row2, const Row& row3) {
            return { row0, row1, row2, row3 };
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

        static constexpr Mat4x4 lookToLH(const Row& eye, const Row& dir, const Row& up) {
            auto r2 = dir.normal();
            auto r0 = Row::cross(up, r2).normal();
            auto r1 = Row::cross(r2, r0);
            auto negEye = eye.negate();

            auto d0 = Row::dot(r0, negEye);
            auto d1 = Row::dot(r1, negEye);
            auto d2 = Row::dot(r2, negEye);

            auto control = Vec4<bool>::from(true, true, true, false);
            auto s0 = Row::select(d0, r0, control);
            auto s1 = Row::select(d1, r1, control);
            auto s2 = Row::select(d2, r2, control);
            auto s3 = Row::from(0, 0, 0, 1);

            return from(s0, s1, s2, s3).transpose();
        }

        static constexpr Mat4x4 lookToRH(const Row& eye, const Row& dir, const Row& up) {
            return lookToLH(eye, dir.negate(), up);
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
}
