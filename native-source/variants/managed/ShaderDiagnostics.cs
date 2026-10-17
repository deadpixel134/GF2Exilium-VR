#if GF2VR_TOUCH_PRIVATE
using System;
using System.Text;
using UnityEngine;

namespace GF2.Private
{
    public static partial class ControllerDisplay
    {
        private sealed class ShaderDiagnostic
        {
            public bool Attempted, Found, Supported, PropertiesRead, Error;
            public bool Surface, SrcBlend, DstBlend, ZWrite;
        }

        private static readonly string[] shaderCandidates = {
            "Universal Render Pipeline/Unlit", "Unlit/Transparent", "Unlit/Color", "Hidden/Internal-Colored"
        };
        private static ShaderDiagnostic[] shaderDiagnostics;
        private static string lastDiagnostics = "{\"event\":\"touch_p2_shader_diagnostics\",\"not_prepared\":true}";

        private static void BeginShaderDiagnostics()
        {
            shaderDiagnostics = new ShaderDiagnostic[shaderCandidates.Length];
            for (int i = 0; i < shaderDiagnostics.Length; ++i) shaderDiagnostics[i] = new ShaderDiagnostic();
        }

        private static void ProbeShaderCandidates()
        {
            // Cold read-only observation. Probe materials are owned temporaries;
            // renderer/material selection and all existing shader controls below
            // remain exactly the original product policy.
            for (int i = 0; i < shaderCandidates.Length; ++i)
            {
                ShaderDiagnostic record = shaderDiagnostics[i];
                Material probe = null;
                try
                {
                    record.Attempted = true;
                    Shader shader = Shader.Find(shaderCandidates[i]);
                    record.Found = shader != null;
                    if (shader == null) continue;
                    record.Supported = shader.isSupported;
                    probe = new Material(shader);
                    probe.hideFlags = HideFlags.HideAndDontSave;
                    record.Surface = probe.HasProperty("_Surface");
                    record.SrcBlend = probe.HasProperty("_SrcBlend");
                    record.DstBlend = probe.HasProperty("_DstBlend");
                    record.ZWrite = probe.HasProperty("_ZWrite");
                    record.PropertiesRead = true;
                }
                catch (Exception) { record.Error = true; }
                finally
                {
                    if (probe != null)
                    {
                        try { UnityEngine.Object.DestroyImmediate(probe); }
                        catch (Exception) { record.Error = true; }
                    }
                }
            }
        }

        private static void DiagnosticBool(StringBuilder text, bool available, bool value)
        { text.Append(!available ? "null" : value ? "true" : "false"); }

        private static void FinishShaderDiagnostics(int prepareStatus)
        {
            try
            {
                StringBuilder text = new StringBuilder(2048);
                text.Append("{\"event\":\"touch_p2_shader_diagnostics\",\"eye_capacity\":").Append(BuildContract.EyeCapacity);
                text.Append(",\"prepare_status\":").Append(prepareStatus).Append(",\"material_mode\":").Append(materialMode);
                text.Append(",\"shader_behavior_changed\":false,\"candidates\":[");
                for (int i = 0; i < shaderCandidates.Length; ++i)
                {
                    if (i != 0) text.Append(',');
                    ShaderDiagnostic record = shaderDiagnostics[i];
                    text.Append("{\"name\":"); Quoted(text, shaderCandidates[i]);
                    text.Append(",\"attempted\":").Append(record.Attempted ? "true" : "false");
                    text.Append(",\"find_null\":"); DiagnosticBool(text, record.Attempted, !record.Found);
                    text.Append(",\"is_supported\":"); DiagnosticBool(text, record.Found, record.Supported);
                    text.Append(",\"has_Surface\":"); DiagnosticBool(text, record.PropertiesRead, record.Surface);
                    text.Append(",\"has_SrcBlend\":"); DiagnosticBool(text, record.PropertiesRead, record.SrcBlend);
                    text.Append(",\"has_DstBlend\":"); DiagnosticBool(text, record.PropertiesRead, record.DstBlend);
                    text.Append(",\"has_ZWrite\":"); DiagnosticBool(text, record.PropertiesRead, record.ZWrite);
                    text.Append(",\"error\":").Append(record.Error ? "true" : "false").Append('}');
                }
                text.Append("]}");
                lastDiagnostics = text.Length <= 4096 ? text.ToString() : "{\"event\":\"touch_p2_shader_diagnostics\",\"error\":\"overflow\"}";
            }
            catch (Exception) { lastDiagnostics = "{\"event\":\"touch_p2_shader_diagnostics\",\"error\":\"diagnostic_exception\"}"; }
        }

        public static string Diagnostics()
        {
            if (!OnOwner()) return "{\"event\":\"touch_p2_shader_diagnostics\",\"error\":\"wrong_thread\"}";
            return lastDiagnostics;
        }
    }
}
#endif
