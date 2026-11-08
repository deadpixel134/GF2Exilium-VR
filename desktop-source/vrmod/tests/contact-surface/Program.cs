using System.Text;
using GF2.Private;
using UnityEngine;

internal static class Program
{
    private static int checks;
    private static void Check(bool result, string label) { ++checks; if (!result) throw new Exception(label); }
    private static void Main()
    {
        var random = new Random(544);
        for (int run=0;run<100;++run) {
            Vector3 expected=new((float)(random.NextDouble()*4-2),(float)(random.NextDouble()*4-2),(float)(random.NextDouble()*4-2));
            float expectedRadius=.02f+(float)random.NextDouble()*.2f;
            var points=new Vector3[512];
            for (int i=0;i<points.Length;++i) {
                double z=2*random.NextDouble()-1, angle=random.NextDouble()*2*Math.PI;
                // Partial surfaces, including a front hemisphere, must recover
                // the known center without treating the centroid as a sphere.
                if ((run&1)==0) z=Math.Abs(z);
                double r=Math.Sqrt(1-z*z);
                points[i]=expected+new Vector3((float)(r*Math.Cos(angle)),(float)(r*Math.Sin(angle)),(float)z)*expectedRadius;
            }
            Check(ContactSurfaceMath.Fit(points,points.Length,out var center,out float radius,out float error),"known sphere fit");
            Check((center-expected).magnitude<2e-5f && Math.Abs(radius-expectedRadius)<2e-5f && error<2e-5f,"independent sphere center/radius/error");
            points[0].x=float.NaN;
            Check(!ContactSurfaceMath.Fit(points,points.Length,out _,out _,out _),"nonfinite rejection");
        }
        var plane=Enumerable.Range(0,32).Select(i=>new Vector3(i%8,i/8,0)).ToArray();
        Check(!ContactSurfaceMath.Fit(plane,32,out _,out _,out _),"planar/ill-conditioned fit rejected");
        Check(!ContactSurfaceMath.Fit(new Vector3[16],16,out _,out _,out _),"collapsed cloud rejected");
        Check(!ContactSurfaceMath.Fit(plane,11,out _,out _,out _),"insufficient samples");
        Check(ContactSurfaceMath.UniformScale(Matrix4x4.Scale(2,2,2),out float uniform)&&uniform==2,"uniform scale");
        Check(ContactSurfaceMath.UniformScale(Matrix4x4.Scale(-2,2,2),out _),"reflected uniform scale");
        Check(!ContactSurfaceMath.UniformScale(Matrix4x4.Scale(2,3,2),out _),"nonuniform avoids false sphere");
        Check(!ContactSurfaceMath.UniformScale(Matrix4x4.Scale(0,0,0),out _),"singular transform");
        ContactSettings settings;
        Check(ContactSettings.Read(null,out settings)&&!settings.ShowContactLocations,"default visualization OFF");
        foreach(bool enabled in new[]{true,false}) {
            Check(ContactSettings.Read(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"showContactLocations\":"+enabled.ToString().ToLowerInvariant()+"}"),out settings)&&settings.ShowContactLocations==enabled,"visualization boolean consumption");
        }
        foreach(string value in new[]{"0","1","null","\"true\"","[]","{}"})
            Check(!ContactSettings.Read(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"showContactLocations\":"+value+"}"),out settings),"invalid visualization type");
        Check(!ContactSettings.Read(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"showContactLocations\":true,\"showContactLocations\":false}"),out settings),"duplicate visualization rejected");
        checks+=PatchTests.Run();
        checks+=RestRecoveryTests.Run();
        checks+=InteriorContactTests.Run();
        checks+=RegionTests.Run();
        checks+=ControllerDisplay.RunRegionRuntimeTests();
        checks+=ControllerDisplay.RunIndentationTests();
        Console.WriteLine(System.Text.Json.JsonSerializer.Serialize(new{passed=true,checks,runtimeAcceptance=false}));
    }
}

// Engine-independent vector host for the production fit. Engine API and
// lifecycle binding are separately compiled against exact game metadata.
namespace UnityEngine
{
    internal struct Vector3
    {
        internal float x,y,z;
        internal Vector3(float x,float y,float z) { this.x=x;this.y=y;this.z=z; }
        internal static Vector3 zero=>new(0,0,0);
        internal static Vector3 right=>new(1,0,0);
        internal static Vector3 up=>new(0,1,0);
        internal static Vector3 forward=>new(0,0,1);
        internal static Vector3 Min(Vector3 a,Vector3 b)=>new(Math.Min(a.x,b.x),Math.Min(a.y,b.y),Math.Min(a.z,b.z));
        internal static Vector3 Max(Vector3 a,Vector3 b)=>new(Math.Max(a.x,b.x),Math.Max(a.y,b.y),Math.Max(a.z,b.z));
        internal float sqrMagnitude=>x*x+y*y+z*z;
        internal static Vector3 Cross(Vector3 a,Vector3 b)=>new(a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x);
        internal float magnitude=>(float)Math.Sqrt((double)x*x+(double)y*y+(double)z*z);
        internal Vector3 normalized=>magnitude>1e-12f?this/magnitude:zero;
        public static Vector3 operator +(Vector3 a,Vector3 b)=>new(a.x+b.x,a.y+b.y,a.z+b.z);
        public static Vector3 operator -(Vector3 a,Vector3 b)=>new(a.x-b.x,a.y-b.y,a.z-b.z);
        public static Vector3 operator *(Vector3 a,float b)=>new(a.x*b,a.y*b,a.z*b);
        public static Vector3 operator /(Vector3 a,float b)=>new(a.x/b,a.y/b,a.z/b);
        internal static float Dot(Vector3 a,Vector3 b)=>a.x*b.x+a.y*b.y+a.z*b.z;
    }
    internal struct BoneWeight
    {
        internal int boneIndex0,boneIndex1,boneIndex2,boneIndex3;
        internal float weight0,weight1,weight2,weight3;
    }
    internal struct Matrix4x4
    {
        private System.Numerics.Matrix4x4 data;
        internal static Matrix4x4 Scale(float x,float y,float z)=>new(){data=System.Numerics.Matrix4x4.CreateScale(x,y,z)};
        internal static Matrix4x4 Translate(float x,float y,float z)=>new(){data=System.Numerics.Matrix4x4.CreateTranslation(x,y,z)};
        internal static Matrix4x4 RotateZ(float radians)=>new(){data=System.Numerics.Matrix4x4.CreateRotationZ(radians)};
        internal float determinant=>data.GetDeterminant();
        internal Matrix4x4 inverse { get { System.Numerics.Matrix4x4.Invert(data,out var result); return new(){data=result}; } }
        internal Matrix4x4 transpose=>new(){data=System.Numerics.Matrix4x4.Transpose(data)};
        public static Matrix4x4 operator *(Matrix4x4 a,Matrix4x4 b)=>new(){data=b.data*a.data};
        public float this[int row,int column]
        {
            get => (column*4+row) switch {
                0=>data.M11,
                1=>data.M12,
                2=>data.M13,
                3=>data.M14,
                4=>data.M21,
                5=>data.M22,
                6=>data.M23,
                7=>data.M24,
                8=>data.M31,
                9=>data.M32,
                10=>data.M33,
                11=>data.M34,
                12=>data.M41,
                13=>data.M42,
                14=>data.M43,
                15=>data.M44,
                _=>throw new IndexOutOfRangeException()
            };
            set { switch(column*4+row) {
                case 0:data.M11=value;break;
                case 1:data.M12=value;break;
                case 2:data.M13=value;break;
                case 3:data.M14=value;break;
                case 4:data.M21=value;break;
                case 5:data.M22=value;break;
                case 6:data.M23=value;break;
                case 7:data.M24=value;break;
                case 8:data.M31=value;break;
                case 9:data.M32=value;break;
                case 10:data.M33=value;break;
                case 11:data.M34=value;break;
                case 12:data.M41=value;break;
                case 13:data.M42=value;break;
                case 14:data.M43=value;break;
                case 15:data.M44=value;break;
                default:throw new IndexOutOfRangeException();
            } }
        }
        internal Vector3 MultiplyVector(Vector3 v){var n=System.Numerics.Vector3.TransformNormal(new(v.x,v.y,v.z),data);return new(n.X,n.Y,n.Z);}
        internal Vector3 MultiplyPoint3x4(Vector3 v){var n=System.Numerics.Vector3.Transform(new(v.x,v.y,v.z),data);return new(n.X,n.Y,n.Z);}
    }
}
namespace GF2.Private
{
    internal static class ContactMotion
    {
        internal static bool Finite(float x)=>!float.IsNaN(x)&&!float.IsInfinity(x);
        internal static bool Finite(Vector3 v)=>Finite(v.x)&&Finite(v.y)&&Finite(v.z);
    }
}
