#if GF2VR_TOUCH_PRIVATE && GF2VR_DIRTY_CONTACT
using System;
using UnityEngine;
namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private static bool ContactExact(Quaternion a,Quaternion b)
        { return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.w==b.w; }
        private static bool ContactRotationValid(Quaternion q)
        {
            if(!ContactMotion.Finite(q))return false;
            Quaternion n=ContactMotion.Normalize(q);
            return n.x*n.x+n.y*n.y+n.z*n.z+n.w*n.w>.9f;
        }

        private static void ContactRestoreOne(int index)
        {
            ContactTarget t=contactTargets[index];
            // Each channel has independent animation ownership. A failure of
            // one engine property must not prevent restoring the other two.
            bool writable=false;
            try { writable=t.Bone!=null&&t.Bone.parent==t.Parent; }catch(Exception){++contactFaults;}
            if(writable) {
                try { if(t.Owned) { if(ContactExact(t.Bone.localPosition,t.LastWritten)){t.Bone.localPosition=t.Base;++contactRestores;}else ++contactExternal; } }catch(Exception){++contactFaults;}
                try { if(t.RotationOwned) { if(ContactExact(t.Bone.localRotation,t.LastRotation)){t.Bone.localRotation=t.BaseRotation;++contactRestores;}else ++contactExternal; } }catch(Exception){++contactFaults;}
                try { if(t.ScaleOwned) { if(ContactExact(t.Bone.localScale,t.LastScale)){t.Bone.localScale=t.BaseScale;++contactRestores;}else ++contactExternal; } }catch(Exception){++contactFaults;}
            }
            t.Owned=t.RotationOwned=t.ScaleOwned=false;
            t.Offset=t.Velocity=t.Tilt=t.TiltVelocity=t.PressLog=Vector3.zero;
            t.InsideMask=0;t.HandTicket=-1;contactTargets[index]=t;
        }

        private static Vector3 ContactCompliant(Vector3 world,ContactTarget t,Vector3 depth,Vector3 vertical,Vector3 lateral)
        {
            if(depth.sqrMagnitude==0)return world;
            return t.Parent.TransformDirection(ContactMotion.Compliance(t.Parent.InverseTransformDirection(world),depth,vertical,lateral,t.Material));
        }

        private static void ContactApply(int index,float dt,bool hands)
        {
            ContactTarget t=contactTargets[index];ContactMaterial m=t.Material;
            Vector3 current=t.Bone.localPosition,scale=t.Bone.localScale;Quaternion rotation=t.Bone.localRotation;
            if(!Finite(current)||!ContactFiniteScale(scale)||!ContactRotationValid(rotation)||
                !ContactFiniteScale(t.Parent.lossyScale)||!ContactFiniteMatrix(t.Parent.localToWorldMatrix))
            {++contactInvalid;contactHapticValid=0;ContactRestoreOne(index);return;}
            if(!t.Owned||!ContactExact(current,t.LastWritten)){if(t.Owned)++contactExternal;t.Base=current;t.Owned=false;}
            if(!t.RotationOwned||!ContactExact(rotation,t.LastRotation)){if(t.RotationOwned)++contactExternal;t.BaseRotation=rotation;t.RotationOwned=false;}
            if(!t.ScaleOwned||!ContactExact(scale,t.LastScale)){if(t.ScaleOwned)++contactExternal;t.BaseScale=scale;t.ScaleOwned=false;}
            Vector3 position=t.Parent.TransformPoint(t.Base);
            if(!Finite(position)){++contactInvalid;contactHapticValid=0;ContactRestoreOne(index);return;}
            float maximum=m.MaxDisplacement*contactUnitScale;
            float radius=contactSettings.HandRadius*contactHandScale+contactSettings.BodyRadius*contactUnitScale;
            if(!Finite(maximum)||!Finite(radius)||radius<=0)
            {++contactInvalid;contactHapticValid=0;ContactRestoreOne(index);return;}
            // Derive geometry from the unmodified basis, never last frame's
            // deformed Transform. No animation delta enters a motion state.
            Vector3 localDirection=t.Child!=null&&t.Child.parent==t.Bone?t.Child.localPosition:Vector3.zero;
            Vector3 direction=t.BaseRotation*Vector3.Scale(t.BaseScale,localDirection);
            float lever=direction.magnitude;
            if(lever==0) {
                direction=t.Base;lever=t.Parent.InverseTransformVector(t.Parent.TransformDirection(ContactMotion.Unit(direction))*contactSettings.BodyRadius*contactUnitScale).magnitude;
                Vector3 unrotated=ContactMotion.Inverse(t.BaseRotation)*direction;
                localDirection=new Vector3(unrotated.x/t.BaseScale.x,unrotated.y/t.BaseScale.y,unrotated.z/t.BaseScale.z);
            }
            Vector3 depth=ContactMotion.Unit(direction),vertical,lateral,outward;
            bool geometry=ContactMotion.Finite(direction)&&ContactMotion.Finite(lever)&&lever>0&&depth.sqrMagnitude>0;
            vertical=lateral=Vector3.zero;
            if(geometry)ContactMotion.Basis(depth,t.Parent.InverseTransformDirection(Vector3.up),t.Parent.InverseTransformDirection(t.Actor.up),out vertical,out lateral);
            else depth=Vector3.zero;
            int axis=geometry?ContactMotion.Axis(localDirection,.7f,out outward):-1;
            outward=Vector3.zero;if(axis>=0)ContactMotion.Axis(localDirection,.7f,out outward);
            t.Alignment=geometry?(axis<0?2:1):3;
            Vector3 desired=Vector3.zero,pressure=Vector3.zero;float penetration=0;int inside=0;
            if(hands) {
                if((contactMask&1)!=0)ContactPressure(position,contactLeft,radius,maximum,ref desired,ref pressure,ref penetration,ref inside,1);
                if((contactMask&2)!=0)ContactPressure(position,contactRight,radius,maximum,ref desired,ref pressure,ref penetration,ref inside,2);
                if(t.HandTicket!=contactTicket) {
                    int released=t.InsideMask&~inside&contactMask&contactVelocityMask;
                    if((released&1)!=0&&m.ReleaseImpulse>0&&contactLeftVelocity.sqrMagnitude>0){t.Velocity+=ContactCompliant(contactLeftVelocity,t,depth,vertical,lateral)*m.ReleaseImpulse;++contactImpulses;}
                    if((released&2)!=0&&m.ReleaseImpulse>0&&contactRightVelocity.sqrMagnitude>0){t.Velocity+=ContactCompliant(contactRightVelocity,t,depth,vertical,lateral)*m.ReleaseImpulse;++contactImpulses;}
                    t.HandTicket=contactTicket;
                }
            }else t.HandTicket=-1;
            t.InsideMask=inside;contactHapticInside|=inside;
            desired=ContactCompliant(ContactClamp(desired,maximum),t,depth,vertical,lateral);
            if(maximum==0){t.Offset=t.Velocity=Vector3.zero;ContactMotion.LastImplicitSteps=ContactMotion.LastStiffnessCaps=0;}
            else if(!ContactMotion.Spring(ref t.Offset,ref t.Velocity,desired,dt,m.Frequency,m.Damping,maximum,m.Stiffening,.001f))
            {++contactInvalid;contactHapticValid=0;ContactRestoreOne(index);return;}
            contactStiffnessCaps+=ContactMotion.LastStiffnessCaps;contactImplicitSteps+=ContactMotion.LastImplicitSteps;
            Vector3 output=ContactMotion.Soft(t.Offset,maximum),localOffset=t.Parent.InverseTransformVector(output);
            Vector3 tiltGoal=geometry?ContactMotion.TiltGoal(depth,localOffset,lever,m.TiltGain,m.MaxTilt*(float)(Math.PI/180)):Vector3.zero;
            if(geometry&&m.MaxTilt>0) {
                if(!ContactMotion.Spring(ref t.Tilt,ref t.TiltVelocity,tiltGoal,dt,m.TiltFrequency,m.TiltDamping,m.MaxTilt*(float)(Math.PI/180),0,.015f))
                {++contactInvalid;contactHapticValid=0;ContactRestoreOne(index);return;}
            }else t.Tilt=t.TiltVelocity=Vector3.zero;
            Vector3 pressGoal=Vector3.zero;
            if(axis>=0&&penetration>0&&m.MaxPressSquash>0) {
                Vector3 n=ContactMotion.Inverse(t.BaseRotation)*t.Parent.InverseTransformDirection(pressure),ignored;
                int pressAxis=ContactMotion.Axis(n,.6f,out ignored);t.PressAlignment=pressAxis<0?2:1;
                if(pressAxis>=0) {
                    float worldLever=t.Parent.TransformVector(depth*lever).magnitude;
                    if(worldLever>0)pressGoal=ContactMotion.AxisLog(pressAxis,1-ContactMotion.SoftScalar((double)m.PressGain*penetration/worldLever,m.MaxPressSquash));
                }
            }else t.PressAlignment=0;
            t.PressLog=axis>=0?ContactMotion.FilterPress(t.PressLog,pressGoal,dt,m.Frequency,m.Damping):Vector3.zero;
            float quietScale=Math.Max(m.MaxSquash,m.MaxPressSquash)*.001f;
            if(pressGoal.sqrMagnitude==0&&ContactMotion.Length(t.PressLog)<=quietScale)t.PressLog=Vector3.zero;
            double extension=axis>=0&&(m.MaxStretch>0||m.MaxSquash>0)?(double)m.StretchGain*Vector3.Dot(localOffset,ContactMotion.Unit(t.BaseRotation*Vector3.Scale(t.BaseScale,outward)))/lever:0;
            Vector3 factors=ContactMotion.ScaleFactors(axis,extension,t.PressLog,m);
            Quaternion finalRotation=ContactMotion.Normalize(t.BaseRotation*ContactMotion.Rotation(ContactMotion.Inverse(t.BaseRotation)*ContactMotion.Soft(t.Tilt,m.MaxTilt*(float)(Math.PI/180))));
            Vector3 final=t.Base+localOffset,finalScale=Vector3.Scale(t.BaseScale,factors);
            if(!Finite(final)||!ContactMotion.Finite(finalRotation)||!ContactFiniteScale(finalScale)||
                !Finite(t.Parent.TransformPoint(final))||!ContactFiniteScale(Vector3.Scale(finalScale,t.Parent.lossyScale)))
            {++contactInvalid;contactHapticValid=0;ContactRestoreOne(index);return;}
            bool positionActive=t.Offset.sqrMagnitude!=0||t.Velocity.sqrMagnitude!=0;
            bool rotationActive=t.Tilt.sqrMagnitude!=0||t.TiltVelocity.sqrMagnitude!=0;
            bool scaleActive=axis>=0&&(extension!=0||t.PressLog.sqrMagnitude!=0);
            if(!positionActive&&!rotationActive&&!scaleActive) {
                contactTargets[index]=t;ContactRestoreOne(index);
                contactTargets[index].InsideMask=t.InsideMask;contactTargets[index].HandTicket=t.HandTicket;return;
            }
            // Stage ownership before engine writes so boundary cleanup can
            // restore a successfully written earlier channel after an exception.
            if(positionActive||t.Owned) {
                t.Owned=true;t.LastWritten=final;contactTargets[index]=t;
                t.Bone.localPosition=final;t.LastWritten=t.Bone.localPosition;++contactWrites;
            }
            if(rotationActive||t.RotationOwned) {
                t.RotationOwned=true;t.LastRotation=finalRotation;contactTargets[index]=t;
                t.Bone.localRotation=finalRotation;t.LastRotation=t.Bone.localRotation;++contactRotationWrites;
            }
            if(scaleActive||t.ScaleOwned) {
                t.ScaleOwned=true;t.LastScale=finalScale;contactTargets[index]=t;
                t.Bone.localScale=finalScale;t.LastScale=t.Bone.localScale;++contactScaleWrites;
            }
            contactTargets[index]=t;
        }

        private static void ContactPressure(Vector3 position,Vector3 hand,float radius,float maximum,
            ref Vector3 desired,ref Vector3 pressure,ref float penetration,ref int inside,int bit)
        {
            Vector3 away=position-hand;float distance=away.magnitude;
            if(!Finite(distance)||distance>=radius)return;
            inside|=bit;
            if(distance<=.000001f)return;
            float p=radius-distance;Vector3 n=away/distance;
            desired+=n*Math.Min(p,maximum);pressure+=n*p;penetration+=p;
        }
    }
}
#endif
