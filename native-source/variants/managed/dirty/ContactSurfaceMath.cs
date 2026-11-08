#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using UnityEngine;

namespace GF2.Private
{
    internal static class ContactSurfaceMath
    {
        internal static bool IncludeTriangle(int region, float a, float b, float c)
        {
            if (!ContactMotion.Finite(a) || !ContactMotion.Finite(b) || !ContactMotion.Finite(c) || a<0 || b<0 || c<0) return false;
            // A butt control may share every vertex with Root/Hip and never
            // exceed50%. Its nonzero skin support still defines real geometry.
            if (region==2) return Math.Max(a,Math.Max(b,c))>0;
            return Math.Max(a,Math.Max(b,c))>=.5f && a>.05f && b>.05f && c>.05f;
        }
        // Cold least-squares sphere fit. Centering and normalization avoid
        // cancellation for small characters at large world coordinates.
        internal static bool Fit(Vector3[] points, int count, out Vector3 center,
            out float radius, out float error)
        {
            center = Vector3.zero; radius = error = 0;
            if (points == null || count < 12 || count > points.Length) return false;
            Vector3 mean = Vector3.zero;
            for (int i = 0; i < count; ++i) {
                if (!ContactMotion.Finite(points[i])) return false;
                mean += points[i] / count;
            }
            double extent = 0;
            for (int i = 0; i < count; ++i) extent = Math.Max(extent, (points[i] - mean).magnitude);
            if (extent < 1e-7 || Double.IsInfinity(extent)) return false;
            double[,] a = new double[4, 5];
            double[] row = new double[4];
            for (int i = 0; i < count; ++i) {
                Vector3 p = (points[i] - mean) / (float)extent;
                row[0] = 2 * p.x; row[1] = 2 * p.y; row[2] = 2 * p.z; row[3] = 1;
                double square = (double)p.x*p.x + (double)p.y*p.y + (double)p.z*p.z;
                for (int j = 0; j < 4; ++j) {
                    for (int k = 0; k < 4; ++k) a[j,k] += row[j] * row[k];
                    a[j,4] += row[j] * square;
                }
            }
            for (int col = 0; col < 4; ++col) {
                int pivot = col;
                for (int r = col + 1; r < 4; ++r) if (Math.Abs(a[r,col]) > Math.Abs(a[pivot,col])) pivot = r;
                if (Math.Abs(a[pivot,col]) < count * 1e-9) return false;
                for (int j = col; j < 5; ++j) { double v = a[col,j]; a[col,j] = a[pivot,j]; a[pivot,j] = v; }
                double divisor = a[col,col];
                for (int j = col; j < 5; ++j) a[col,j] /= divisor;
                for (int r = 0; r < 4; ++r) if (r != col) {
                    double factor = a[r,col];
                    for (int j = col; j < 5; ++j) a[r,j] -= factor * a[col,j];
                }
            }
            double squaredRadius = a[0,4]*a[0,4] + a[1,4]*a[1,4] + a[2,4]*a[2,4] + a[3,4];
            if (squaredRadius <= 0 || Double.IsInfinity(squaredRadius) || Double.IsNaN(squaredRadius)) return false;
            center = mean + new Vector3((float)a[0,4], (float)a[1,4], (float)a[2,4]) * (float)extent;
            radius = (float)(Math.Sqrt(squaredRadius) * extent);
            if (!ContactMotion.Finite(center) || !ContactMotion.Finite(radius) || radius <= 0) return false;
            // Maximum radial residual over every retained sample, not merely
            // the fitted subset. Rejection keeps unproved proxies out of use.
            for (int i = 0; i < count; ++i) error = Math.Max(error, Math.Abs((points[i] - center).magnitude - radius));
            return ContactMotion.Finite(error);
        }

        internal static bool UniformScale(Matrix4x4 matrix, out float scale)
        {
            Vector3 x = matrix.MultiplyVector(Vector3.right);
            Vector3 y = matrix.MultiplyVector(Vector3.up);
            Vector3 z = matrix.MultiplyVector(Vector3.forward);
            scale = x.magnitude;
            if (!ContactMotion.Finite(scale) || scale <= 0 || !ContactMotion.Finite(y) || !ContactMotion.Finite(z)) return false;
            float tolerance = scale * .001f;
            return Math.Abs(y.magnitude-scale) <= tolerance && Math.Abs(z.magnitude-scale) <= tolerance &&
                Math.Abs(Vector3.Dot(x,y)) <= scale*scale*.001f &&
                Math.Abs(Vector3.Dot(x,z)) <= scale*scale*.001f && Math.Abs(Vector3.Dot(y,z)) <= scale*scale*.001f;
        }
    }
}
#endif
