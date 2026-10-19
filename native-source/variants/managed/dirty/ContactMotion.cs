#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using UnityEngine;
namespace GF2.Private
{
    // Engine-free math; all vectors are explicit in their caller-owned basis.
    internal static class ContactMotion
    {
        internal static int LastSubsteps,LastStiffnessCaps,LastImplicitSteps;
        internal static float LastOmegaStep;
        internal static bool Spring(ref Vector3 offset,ref Vector3 velocity,Vector3 desired,float dt,
            float frequency,float damping,float maximum,float stiffening,float quietRelative)
        {
            LastSubsteps=LastStiffnessCaps=LastImplicitSteps=0;LastOmegaStep=0;
            if(!Finite(offset)||!Finite(velocity)||!Finite(desired)||!Finite(dt)||!Finite(frequency)||!Finite(damping)||
                !Finite(maximum)||!Finite(stiffening)||dt<=0||frequency<0||damping<0||maximum<=0||stiffening<0)
            {offset=velocity=Vector3.zero;return false;}
            if(frequency==0){offset=velocity=Vector3.zero;return true;}
            double omega=2*Math.PI*frequency;
            double wanted=Math.Max(Math.Ceiling(Math.Min(dt,.05f)*480),Math.Ceiling(Math.Min(dt,.05f)*omega*Math.Sqrt(1+27*(double)stiffening)/.499));
            int steps=(int)Math.Max(1,Math.Min(32,wanted));LastSubsteps=steps;
            double h=Math.Min(dt,.05f)/steps,w=omega*h,c=2*(double)damping*w,w2=w*w;
            // Work in units of the user-selected L, with double intermediates.
            // Avoid float overflow even where a representable state / tiny L
            // has a ratio larger than a float can hold.
            double x=offset.x/(double)maximum,y=offset.y/(double)maximum,z=offset.z/(double)maximum;
            double vx=velocity.x/(double)maximum,vy=velocity.y/(double)maximum,vz=velocity.z/(double)maximum;
            double dx=desired.x/(double)maximum,dy=desired.y/(double)maximum,dz=desired.z/(double)maximum;
            for(int i=0;i<steps;++i){
                double jac=1+3*(double)stiffening*(x*x+y*y+z*z),phase=w*Math.Sqrt(jac);
                LastOmegaStep=Math.Max(LastOmegaStep,(float)Math.Min(Single.MaxValue,phase));
                if(phase<.499&&c<.9){
                    // Ordinary presets retain symplectic Euler. No arbitrary
                    // velocity or3L state clamp truncates user-selected motion.
                    double restoration=omega*omega*(1+(double)stiffening*(x*x+y*y+z*z));
                    double ex=vx+(omega*omega*dx-restoration*x-2*(double)damping*omega*vx)*h;
                    double ey=vy+(omega*omega*dy-restoration*y-2*(double)damping*omega*vy)*h;
                    double ez=vz+(omega*omega*dz-restoration*z-2*(double)damping*omega*vz)*h;
                    double nx=x+ex*h,ny=y+ey*h,nz=z+ez*h;
                    double nextPhase=w*Math.Sqrt(1+3*(double)stiffening*(nx*nx+ny*ny+nz*nz));
                    // A large strike can cross into the nonlinear stiff region
                    // within one step. Validate the tentative state as well,
                    // before committing any explicit update.
                    if(nextPhase<.499){vx=ex;vy=ey;vz=ez;x=nx;y=ny;z=nz;continue;}
                }
                {
                    // Backward Euler for the same radial progressive force.
                    // x_new*(a+b*|x_new|²)=rhs has one positive radial root.
                    // Implicit damping/stiffness is stable without capping the
                    // requested frequency, damping, strength or material gain.
                    ++LastImplicitSteps;double a=1+c+w2;
                    double rx=((1+c)*x+h*vx+w2*dx)/a,ry=((1+c)*y+h*vy+w2*dy)/a,rz=((1+c)*z+h*vz+w2*dz)/a;
                    double alpha=(w2/a)*stiffening*(rx*rx+ry*ry+rz*rz),factor=1;
                    if(alpha>1e-8){
                        double root=Math.Sqrt(alpha),arg=1.5*Math.Sqrt(3)*root;
                        double asinh=Math.Log(arg+Math.Sqrt(arg*arg+1));
                        factor=(2/Math.Sqrt(3))*Math.Sinh(asinh/3)/root;
                    }else factor=1-alpha;
                    double nx=rx*factor,ny=ry*factor,nz=rz*factor;
                    vx=(nx-x)/h;vy=(ny-y)/h;vz=(nz-z)/h;x=nx;y=ny;z=nz;
                }
                if(Double.IsNaN(x)||Double.IsInfinity(x)||Double.IsNaN(y)||Double.IsInfinity(y)||Double.IsNaN(z)||Double.IsInfinity(z))
                {offset=velocity=Vector3.zero;return false;}
            }
            if(desired.x==0&&desired.y==0&&desired.z==0&&x*x+y*y+z*z+(vx*vx+vy*vy+vz*vz)/(omega*omega)<(double)quietRelative*quietRelative)
                x=y=z=vx=vy=vz=0;
            Vector3 next=new Vector3((float)(x*maximum),(float)(y*maximum),(float)(z*maximum));
            Vector3 speed=new Vector3((float)(vx*maximum),(float)(vy*maximum),(float)(vz*maximum));
            if(!Finite(next)||!Finite(speed)){offset=velocity=Vector3.zero;return false;}
            offset=next;velocity=speed;return true;
        }
        internal static double Length(Vector3 v)
        {return Math.Sqrt((double)v.x*v.x+(double)v.y*v.y+(double)v.z*v.z);}
        internal static bool Finite(float x) { return !Single.IsNaN(x)&&!Single.IsInfinity(x); }
        internal static bool Finite(Vector3 v) { return Finite(v.x)&&Finite(v.y)&&Finite(v.z); }
        internal static bool Finite(Quaternion q) { return Finite(q.x)&&Finite(q.y)&&Finite(q.z)&&Finite(q.w); }
        internal static Vector3 Clamp(Vector3 v,float limit)
        {
            if(!Finite(v)||!Finite(limit)||limit<=0)return Vector3.zero;
            double n=Length(v),factor=n>limit?limit/n:1;
            return new Vector3((float)(v.x*factor),(float)(v.y*factor),(float)(v.z*factor));
        }
        internal static Vector3 Unit(Vector3 v)
        {
            if(!Finite(v))return Vector3.zero;double n=Length(v);if(n==0)return Vector3.zero;
            return new Vector3((float)(v.x/n),(float)(v.y/n),(float)(v.z/n));
        }
        internal static Vector3 Soft(Vector3 v,float limit)
        {
            if(!Finite(v)||!Finite(limit)||limit<=0)return Vector3.zero;
            double n=Length(v);if(n==0)return v;
            double factor=limit*.999999*Math.Tanh(n/limit)/n;
            return new Vector3((float)(v.x*factor),(float)(v.y*factor),(float)(v.z*factor));
        }
        internal static float SoftScalar(double value,float limit)
        {return limit>0?(float)(limit*.999999*Math.Tanh(value/limit)):0;}
        internal static Quaternion Normalize(Quaternion q)
        {
            double n=(double)q.x*q.x+(double)q.y*q.y+(double)q.z*q.z+(double)q.w*q.w;
            if(!Finite(q)||n<.000000000001)return new Quaternion(0,0,0,0);
            float inverse=(float)(1/Math.Sqrt(n));return new Quaternion(q.x*inverse,q.y*inverse,q.z*inverse,q.w*inverse);
        }
        internal static Quaternion Inverse(Quaternion q)
        { q=Normalize(q);return new Quaternion(-q.x,-q.y,-q.z,q.w); }
        internal static Quaternion Rotation(Vector3 radians)
        {
            double angle=Length(radians);if(angle==0)return Quaternion.identity;
            double factor=Math.Sin(angle*.5)/angle;
            return Normalize(new Quaternion((float)(radians.x*factor),(float)(radians.y*factor),(float)(radians.z*factor),(float)Math.Cos(angle*.5)));
        }
        internal static bool Basis(Vector3 depth,Vector3 up,Vector3 actorUp,out Vector3 vertical,out Vector3 lateral)
        {
            depth=Unit(depth);vertical=lateral=Vector3.zero;if(depth.sqrMagnitude==0)return false;
            Vector3 v=up-depth*Vector3.Dot(up,depth);
            if(v.magnitude<.2f)v=actorUp-depth*Vector3.Dot(actorUp,depth);
            // Both up vectors can be collinear for a vertical bone. A stable
            // least-aligned parent axis completes the frame without NaN.
            if(v.magnitude<.2f) {
                Vector3 seed=Math.Abs(depth.x)<.7f?new Vector3(1,0,0):new Vector3(0,0,1);
                v=seed-depth*Vector3.Dot(seed,depth);
            }
            vertical=Unit(v);lateral=Unit(Vector3.Cross(depth,vertical));return lateral.sqrMagnitude!=0;
        }
        internal static Vector3 Compliance(Vector3 value,Vector3 depth,Vector3 vertical,Vector3 lateral,ContactMaterial m)
        { return depth*(Vector3.Dot(value,depth)*m.Depth)+vertical*(Vector3.Dot(value,vertical)*m.Vertical)+lateral*(Vector3.Dot(value,lateral)*m.Lateral); }
        internal static int Axis(Vector3 direction,float threshold,out Vector3 outward)
        {
            direction=Unit(direction);outward=Vector3.zero;
            float x=Math.Abs(direction.x),y=Math.Abs(direction.y),z=Math.Abs(direction.z);
            int axis=x>=y&&x>=z?0:y>=z?1:2;float component=axis==0?direction.x:axis==1?direction.y:direction.z;
            if(Math.Abs(component)<threshold)return -1;
            float sign=component<0?-1:1;outward=axis==0?new Vector3(sign,0,0):axis==1?new Vector3(0,sign,0):new Vector3(0,0,sign);return axis;
        }
        internal static Vector3 TiltGoal(Vector3 depth,Vector3 offset,float lever,float gain,float maximum)
        {
            Vector3 perpendicular=offset-depth*Vector3.Dot(depth,offset);
            Vector3 axis=Unit(Vector3.Cross(depth,perpendicular));
            if(!Finite(lever) || lever<=0 || maximum<=0)return Vector3.zero;
            float angle=Math.Min(maximum,(float)(gain*Math.Atan(perpendicular.magnitude/lever)));
            return axis*angle;
        }
        internal static Vector3 AxisLog(int axis,float primary)
        {
            if(axis<0 || primary<=0)return Vector3.zero;
            float a=(float)Math.Log(primary),b=-a*.5f;
            return axis==0?new Vector3(a,b,b):axis==1?new Vector3(b,a,b):new Vector3(b,b,a);
        }
        internal static Vector3 FilterPress(Vector3 current,Vector3 target,float dt,float frequency,float damping)
        {
            float alpha=(float)(1-Math.Exp(-(double)dt*damping*2*Math.PI*frequency));
            Vector3 next=current+(target-current)*alpha;
            return Finite(next)?next:Vector3.zero;
        }
        internal static Vector3 ScaleFactors(int primaryAxis,double extension,Vector3 pressLog,ContactMaterial m)
        {
            if(primaryAxis<0)return new Vector3(1,1,1);
            float strain=extension>=0?SoftScalar(extension,m.MaxStretch):-SoftScalar(-extension,m.MaxSquash);
            Vector3 logs=AxisLog(primaryAxis,1+strain)+pressLog;
            float main=primaryAxis==0?logs.x:primaryAxis==1?logs.y:logs.z;
            float press=primaryAxis==0?pressLog.x:primaryAxis==1?pressLog.y:pressLog.z;
            if(main<0 && press<0) {
                float cap=Math.Max(m.MaxSquash,m.MaxPressSquash);
                float total=1-(float)Math.Exp(main),limited=SoftScalar(total,cap);
                float corrected=(float)Math.Log(1-limited),delta=corrected-main;
                logs=logs-AxisLog(primaryAxis,(float)Math.Exp(-delta));
            }
            // Attenuate logarithmic strain before the last safety clamp. This
            // keeps determinant1 even where two different axes combine. Only
            // float representation bounds remain; no0.6..1.5 product cap.
            double factor=1,lo=Math.Log(Single.Epsilon),hi=Math.Log(Single.MaxValue);
            float max=Math.Max(logs.x,Math.Max(logs.y,logs.z)),min=Math.Min(logs.x,Math.Min(logs.y,logs.z));
            if(max>hi)factor=Math.Min(factor,hi/max);if(min<lo)factor=Math.Min(factor,lo/min);
            return new Vector3((float)Math.Max(Single.Epsilon,Math.Min(Single.MaxValue,Math.Exp(logs.x*factor))),
                (float)Math.Max(Single.Epsilon,Math.Min(Single.MaxValue,Math.Exp(logs.y*factor))),
                (float)Math.Max(Single.Epsilon,Math.Min(Single.MaxValue,Math.Exp(logs.z*factor))));
        }
    }
}
#endif
