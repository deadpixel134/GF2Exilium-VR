using System.Text;
using GF2.Private;
using UnityEngine;

internal static class RegionTests
{
    internal static int Run()
    {
        int checks=0;
        void Check(bool result,string label) { ++checks; if (!result) throw new Exception(label); }
        ContactSettings settings;
        Check(ContactSettings.Read(null,out settings) && settings.Targets.Length==7,"seven default contact targets");
        foreach(string name in new[]{"Head_M","Hip_L","Hip_R"})
            Check(ContactSettings.DetectionOnly(ContactSettings.Region(name)),"new region detection-only policy");
        Check(settings.ForBone("Head_M").MaxDisplacement==0 && settings.ForBone("Hip_L").ReleaseImpulse==0,"new regions never inherit tissue material");
        Check(ContactSettings.PreferSurfacePatch(2,1),"valid butt triangle patch overrides eligible sphere fit");
        Check(!ContactSettings.PreferSurfacePatch(1,1),"accepted breast sphere policy preserved");
        foreach(int region in new[]{1,2,3,4,5}) {
            Check(ContactSettings.PreferSurfacePatch(region,4),"rejected sphere selects available triangle patch");
            if(region>=3) Check(ContactSettings.PreferSurfacePatch(region,1),"face and thigh always prefer actual patch");
        }
        foreach(string name in new[]{"Chest_L","Butt_L_Back01","Neck_M","HipPart1_L","Knee_L",""})
            Check(!ContactSettings.DetectionOnly(ContactSettings.Region(name)),"old and unrequested targets retain policy");
        string standard="[\"Chest_R\",\"Butt_R_Back01\",\"Chest_L\",\"Butt_L_Back01\"]";
        Check(ContactSettings.Read(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"targetBones\":"+standard+"}"),out settings) &&
            settings.Targets.SequenceEqual(new[]{"Chest_L","Chest_R","Butt_L_Back01","Butt_R_Back01","Head_M","Hip_L","Hip_R"}),"legacy complete standard list upgraded independent of order");
        foreach(string list in new[]{"[]","[\"Chest_L\"]","[\"Head_M\",\"Hip_R\"]","[\"Chest_L\",\"Chest_R\",\"Butt_L_Back01\",\"Hip_L\"]"}) {
            Check(ContactSettings.Read(Encoding.UTF8.GetBytes("{\"schemaVersion\":1,\"targetBones\":"+list+"}"),out settings),"custom list parses");
            Check(settings.Targets.SequenceEqual(System.Text.Json.JsonSerializer.Deserialize<string[]>(list)!),"custom/empty lists retain exact contents");
        }

        Vector3[] first=new Vector3[6], second=new Vector3[6], normals=new Vector3[6], otherNormals=new Vector3[6];
        var random=new Random(549);
        for(int run=0;run<500;++run) {
            Vector3 start=new((float)random.NextDouble(),(float)random.NextDouble(),(float)random.NextDouble());
            Vector3 axis=new(.1f+(float)random.NextDouble(),.1f+(float)random.NextDouble(),.1f+(float)random.NextDouble());
            Vector3 Point()=>start+axis*((float)random.NextDouble()*3-1)+new Vector3((float)random.NextDouble(),(float)random.NextDouble(),(float)random.NextDouble());
            int count=ContactRegionMath.ClipTriangle(Point(),Point(),Point(),Vector3.forward,Vector3.forward,Vector3.forward,
                start,start+axis,first,second,normals,otherNormals);
            Check(count>=0 && count<=5,"parallel-plane clipped polygon capacity");
            for(int i=0;i<count;++i) {
                float t=Vector3.Dot(first[i]-start,axis)/axis.sqrMagnitude;
                Check(t>=-2e-6f && t<=1+2e-6f,"clipped point remains in animated hip-knee segment");
                Check((normals[i]-Vector3.forward).sqrMagnitude<1e-10f,"clipped normals preserve interpolation");
            }
        }
        Check(ContactRegionMath.ClipTriangle(Vector3.zero,Vector3.up,Vector3.right,Vector3.forward,Vector3.forward,Vector3.forward,
            Vector3.zero,Vector3.zero,first,second,normals,otherNormals)==0,"zero-length limb rejected");
        foreach(float a in new[]{-1f,0f,1f,2f}) foreach(float b in new[]{-1f,0f,1f,2f}) foreach(float c in new[]{-1f,0f,1f,2f}) {
            int count=ContactRegionMath.ClipTriangle(new(0,a,0),new(1,b,0),new(0,c,1),Vector3.forward,Vector3.forward,Vector3.forward,
                Vector3.zero,Vector3.up,first,second,normals,otherNormals);
            Check(count<=5,"exact joint-plane vertices fit fixed polygon capacity");
        }

        // A thigh-like open side wall exercises deep contact near both open
        // ends. A single midpoint cone leaves gaps along that interior axis.
        Vector3[] wall={new(-.1f,-.2f,.1f),new(.1f,-.2f,.1f),new(.1f,1.2f,.1f),new(-.1f,1.2f,.1f)};
        int[] input={0,1,2,0,2,3};
        var vertices=new List<Vector3>();
        for(int i=0;i<input.Length;i+=3) {
            int count=ContactRegionMath.ClipTriangle(wall[input[i]],wall[input[i+1]],wall[input[i+2]],
                Vector3.forward,Vector3.forward,Vector3.forward,Vector3.zero,Vector3.up,first,second,normals,otherNormals);
            for(int t=1;t<count-1;++t) { vertices.Add(first[0]);vertices.Add(first[t]);vertices.Add(first[t+1]); }
        }
        Vector3[] points=vertices.ToArray(), outward=Enumerable.Repeat(Vector3.forward,points.Length).ToArray();
        int[] indices=Enumerable.Range(0,points.Length+3).ToArray(); // Unused capacity must never be sampled.
        Vector3 minimum=new(-.1f,0,0), maximum=new(.1f,1,.1f);
        bool Sample(Vector3 hand)=>ContactPatchMath.SampleSegment(points,outward,indices,points.Length,
            Vector3.zero,Vector3.up,minimum,maximum,hand,.035f,out _,out _,out _);
        foreach(float height in new[]{.05f,.2f,.5f,.8f,.95f}) {
            Check(Sample(new(0,height,.12f)),"thigh exterior contact along segment");
            Check(Sample(new(0,height,.01f)),"thigh deep contact retained near both joints");
        }
        Check(!Sample(new(0,-.2f,.1f)) && !Sample(new(0,1.2f,.1f)),"clipped geometry does not contact beyond hip/knee");
        Check(!Sample(new(0,.5f,.15f)),"complete exterior separation");
        Check(!Sample(new(.2f,.5f,.01f)),"finite side boundary");
        Check(!Sample(new(0,.5f,-.1f)),"finite rear boundary");
        Check(ContactPatchMath.SampleSegment(points,outward,indices,0,Vector3.zero,Vector3.up,minimum,maximum,
            new(0,.5f,.1f),.035f,out _,out _,out _)==false,"empty clipped geometry skips unused index capacity");
        return checks;
    }
}
