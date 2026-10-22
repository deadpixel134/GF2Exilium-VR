#if GF2VR_TOUCH_PRIVATE
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Threading;
using UnityEngine;
using UnityEngine.Rendering;

namespace GF2.Private
{
    // Native-admitted source owns display. Typed actor ownership independently
    // gates contact, including the unchanged before-render self-restorer.
    public static partial class ControllerDisplay
    {
        // 1 transparent success; 0 hidden/rejected; -6 usable opaque fallback.
        // -1 wrong thread; -2 cold setup; -3 mesh; -5 material;
        // -7 invalid value; -8 layer excluded; -9 draw failure.
        // -40 unprepared; -41 resources lost; -42 source mismatch;
        // -43 epoch mismatch; -44 generation mismatch; -45 negative ticket;
        // -46 invalid camera pair; -47 pair capacity exhausted.
        private static int ownerThread;
#if GF2VR_DIRTY_CONTACT
        private static Context prepared;
#endif
        private static Camera preparedSource;
        private static readonly Camera[] leftEyes = new Camera[BuildContract.EyeCapacity];
        private static readonly Camera[] rightEyes = new Camera[BuildContract.EyeCapacity];
        private static readonly long[] lastPair = new long[BuildContract.EyeCapacity];
        private static Mesh ownedMesh;
        private static Material ownedMaterial;
        private static GameObject donor;
        private static long ownerEpoch, ownerGeneration;
        private static int materialMode, drawLayer;
        private static float primitiveRadius;
#if GF2VR_DIRTY_CONTACT
        private static int selectKind;
        private static string selectReason = "no-owner";
        private static string mainReject = "no-owner", dormReject = "no-owner", activityReject = "no-owner";

        private sealed class Context
        {
            public int Kind; // 1 Main, 2 Dorm, 3 Activity, 4 legacy actor, 5 source with independent actors.
            public GameObject Root;
            public Camera OwnerCamera;
            public T0BGHelper Main;
            public LoungeHelper Dorm;
            public Lobby.World.LobbyHelper Activity;
            public Animator ContextActor;
            public Transform ActorParent, AnchorBone;
            public SkinnedMeshRenderer AnchorSkin;
            public Mesh AnchorMesh;
        }

#endif

        private static bool OnOwner()
        {
            int current = Thread.CurrentThread.ManagedThreadId;
            if (ownerThread == 0) ownerThread = current;
            return ownerThread == current;
        }

#if GF2VR_DIRTY_CONTACT
        private static bool Live(Behaviour value)
        {
            return value != null && value.isActiveAndEnabled && value.gameObject.activeInHierarchy;
        }

        private static bool Eligible(Context value, Camera source)
        {
            if (value == null || source == null || !source.gameObject.activeInHierarchy ||
                value.Root == null || !value.Root.activeInHierarchy) return false;
            if (value.Kind == 1)
                return Live(value.Main) && value.Main == T0BGHelper.MJGPKADBALF &&
                    T0BGHelper.CurrActive && !value.Main.isGachaModel && value.OwnerCamera == source;
            if (value.Kind == 2)
            {
                if (!Live(value.Dorm) || value.Dorm != LoungeHelper.MJGPKADBALF) return false;
                LoungeFreeCameraControllerNew controller = LoungeHelper.GCKKGOJLMGB;
                return Live(controller) && controller.CameraObj != null &&
                    controller.CameraObj.Camera == source;
            }
            if (value.Kind == 3)
                return Live(value.Activity) &&
                    !value.Activity.isAmo3DSpecialPlot && !Lobby.World.LobbyHelper.isSceneReloading &&
                    !Lobby.World.LobbyHelper.isSceneDayNightSwitching && value.Activity.CameraMgr != null &&
                    value.Activity.CameraMgr.EEMIJPMBCPJ == source;
            if (value.Kind == 5) return value.OwnerCamera == source && value.Root == source.gameObject;
            if (value.Kind == 4) return ContextActorEligible(value, source);
            return false;
        }

        // Cold exact-type lookup only. The MonoSingleton getter is not called:
        // its lazy creation side effects have not been proved absent.
        private static string ContactRejection(Context value, Camera source)
        {
            if (source == null || !source.gameObject.activeInHierarchy) return "invalid-source";
            if (value.Root == null || !value.Root.activeInHierarchy) return "inactive-root";
            if (value.Kind == 5) return Eligible(value, source) ? "eligible-multiple-actors" : "invalid-source";
            if (value.Kind == 4) return ContextActorEligible(value, source) ? "eligible" : "context-actor-invalid";
            if (value.Kind == 1)
            {
                if (!Live(value.Main) || value.Main != T0BGHelper.MJGPKADBALF) return "inactive-owner";
                if (!T0BGHelper.CurrActive) return "CurrActive";
                if (value.Main.isGachaModel) return "gacha";
                return value.OwnerCamera != source ? "OwnerCamera-mismatch" : "eligible";
            }
            if (value.Kind == 2)
            {
                if (!Live(value.Dorm) || value.Dorm != LoungeHelper.MJGPKADBALF) return "inactive-owner";
                LoungeFreeCameraControllerNew controller = LoungeHelper.GCKKGOJLMGB;
                return !Live(controller) || controller.CameraObj == null || controller.CameraObj.Camera != source
                    ? "OwnerCamera-mismatch" : "eligible";
            }
            if (!Live(value.Activity)) return "inactive-owner";
            if (value.Activity.isAmo3DSpecialPlot) return "special-plot";
            if (Lobby.World.LobbyHelper.isSceneReloading || Lobby.World.LobbyHelper.isSceneDayNightSwitching) return "transition";
            return value.Activity.CameraMgr == null || value.Activity.CameraMgr.EEMIJPMBCPJ != source
                ? "OwnerCamera-mismatch" : "eligible";
        }

        private static Context Select(Camera source)
        {
            Context selected = null;
            int matches = 0;
            selectKind = 0; selectReason = mainReject = dormReject = activityReject = "no-owner";
            T0BGHelper main = T0BGHelper.MJGPKADBALF;
            if (main != null)
            {
                // IJOKONJJNCN's getter is lazy and writes the game's cache.
                // Read the exact existing field once instead. Null is rejected.
                FieldInfo cameraField = typeof(T0BGHelper).GetField("LGKDOCICKCP", BindingFlags.NonPublic | BindingFlags.Instance);
                Camera camera = cameraField == null ? null : cameraField.GetValue(main) as Camera;
                Context candidate = new Context { Kind = 1, Root = main.gameObject, Main = main, OwnerCamera = camera };
                mainReject = ContactRejection(candidate, source);
                if (Eligible(candidate, source)) { selected = candidate; ++matches; }
            }
            LoungeHelper dorm = LoungeHelper.MJGPKADBALF;
            if (dorm != null)
            {
                Context candidate = new Context { Kind = 2, Root = dorm.gameObject, Dorm = dorm };
                dormReject = ContactRejection(candidate, source);
                if (Eligible(candidate, source)) { selected = candidate; ++matches; }
            }
            UnityEngine.Object[] activities = UnityEngine.Object.FindObjectsOfType(typeof(Lobby.World.LobbyHelper));
            if (activities != null && activities.Length <= 8)
                for (int i = 0; i < activities.Length; ++i)
                {
                    Lobby.World.LobbyHelper activity = activities[i] as Lobby.World.LobbyHelper;
                    if (activity == null) continue;
                    Context candidate = new Context { Kind = 3, Root = activity.gameObject, Activity = activity };
                    activityReject = ContactRejection(candidate, source);
                    if (Eligible(candidate, source)) { selected = candidate; ++matches; }
                }
            else if (activities != null) { selectReason = activityReject = "owner-capacity"; return null; }
            if (matches == 1)
            {
                // A Lobby helper is a camera owner, not a common parent for
                // every character rendered by that camera.
                if (selected.Kind == 3) selected = SelectContextActor(source);
                if (selected != null) { selectKind = selected.Kind; selectReason = "eligible"; }
                return selected;
            }
            if (matches == 0)
            {
                Context contextual = SelectContextActor(source);
                if (contextual != null) { selectKind = 4; selectReason = "eligible-context-actor"; return contextual; }
            }
            selectReason = matches > 1 ? "ambiguous-owner" : mainReject != "no-owner" ? mainReject :
                dormReject != "no-owner" ? dormReject : activityReject;
            return null;
        }

        // A broken optional game contact binding must not clear valid hand
        // mesh/material resources. This cold wrapper also covers JIT binding
        // failures before the contact core enters its own body.
        private static void RetryContact(Camera source)
        {
            try { RetryContactCore(source); }
            catch (Exception)
            {
                try { ClearContact(); } catch (Exception) { }
                prepared = null;
                selectReason = "contact-prepare-exception";
            }
        }

        // Native Prepare is called on source admission, then at most once per
        // second for this same tuple. Resource/eye state survives contact retries.
        private static void RetryContactCore(Camera source)
        {
            if (prepared != null && prepared.Kind == 4)
            {
                // Native calls this cold path at most once per second. Recheck
                // uniqueness/context without resetting an unchanged spring.
                Context next = Select(source);
                if (next != null && next.Kind == 4 && next.ContextActor == prepared.ContextActor &&
                    next.ActorParent == prepared.ActorParent && Eligible(next, source))
                { prepared = next; if (!contactRunning) BeginContact(); return; }
                ClearContact(); prepared = next;
                if (prepared != null && Eligible(prepared, source)) BeginContact();
                return;
            }
            if (prepared != null && Eligible(prepared, source))
            { if (!contactRunning) BeginContact(); return; }
            ClearContact(); prepared = null;
            Context selected = Select(source);
            if (selected == null || !Eligible(selected, source)) return;
            prepared = selected;
            BeginContact();
        }

#endif

        private static bool Finite(float value)
        { return !Single.IsNaN(value) && !Single.IsInfinity(value); }
        private static bool Finite(Vector3 value)
        { return Finite(value.x) && Finite(value.y) && Finite(value.z); }

        private static Shader Supported(string name)
        {
            Shader shader = Shader.Find(name);
            return shader != null && shader.isSupported ? shader : null;
        }

        private static void IntProperty(Material value, string property, int number)
        { if (value.HasProperty(property)) value.SetInt(property, number); }

        private static void ColorProperty(Material value, string property, Color color)
        { if (value.HasProperty(property)) value.SetColor(property, color); }

        private static bool CreateMaterial()
        {
            Shader shader = Supported("Universal Render Pipeline/Unlit");
            bool urp = shader != null;
            if (shader == null) shader = Supported("Unlit/Transparent");
            if (shader != null)
            {
                ownedMaterial = new Material(shader);
                ownedMaterial.hideFlags = HideFlags.HideAndDontSave;
                bool colorProperty = ownedMaterial.HasProperty("_BaseColor") || ownedMaterial.HasProperty("_Color");
                bool alphaControls = !urp || (ownedMaterial.HasProperty("_Surface") &&
                    ownedMaterial.HasProperty("_SrcBlend") && ownedMaterial.HasProperty("_DstBlend") &&
                    ownedMaterial.HasProperty("_ZWrite"));
                if (!colorProperty || !alphaControls)
                { UnityEngine.Object.DestroyImmediate(ownedMaterial); ownedMaterial = null; shader = null; }
                else
                {
                    IntProperty(ownedMaterial, "_Surface", 1);
                    IntProperty(ownedMaterial, "_Blend", 0);
                    IntProperty(ownedMaterial, "_SrcBlend", (int)BlendMode.SrcAlpha);
                    IntProperty(ownedMaterial, "_DstBlend", (int)BlendMode.OneMinusSrcAlpha);
                    IntProperty(ownedMaterial, "_ZWrite", 0);
                    IntProperty(ownedMaterial, "_ZTest", (int)CompareFunction.LessEqual);
                    IntProperty(ownedMaterial, "_Cull", (int)CullMode.Off);
                    IntProperty(ownedMaterial, "_ReceiveShadows", 0);
                    Color color = new Color(1f, 1f, 1f, 0.35f);
                    ColorProperty(ownedMaterial, "_BaseColor", color);
                    ColorProperty(ownedMaterial, "_Color", color);
                    ownedMaterial.SetOverrideTag("RenderType", "Transparent");
                    ownedMaterial.EnableKeyword("_SURFACE_TYPE_TRANSPARENT");
                    ownedMaterial.EnableKeyword("_ALPHABLEND_ON");
                    ownedMaterial.renderQueue = 3000;
                    ownedMaterial.SetShaderPassEnabled("ShadowCaster", false);
                    ownedMaterial.enableInstancing = false;
                    materialMode = 1;
                    return true;
                }
            }
            shader = Supported("Unlit/Color");
            bool internalShader = shader == null;
            if (shader == null) shader = Supported("Hidden/Internal-Colored");
            if (shader == null) return false;
            ownedMaterial = new Material(shader);
            ownedMaterial.hideFlags = HideFlags.HideAndDontSave;
            if (!ownedMaterial.HasProperty("_Color") && !ownedMaterial.HasProperty("_BaseColor"))
            { UnityEngine.Object.DestroyImmediate(ownedMaterial); ownedMaterial = null; return false; }
            if (internalShader && !ownedMaterial.HasProperty("_ZTest"))
            { UnityEngine.Object.DestroyImmediate(ownedMaterial); ownedMaterial = null; return false; }
            IntProperty(ownedMaterial, "_SrcBlend", (int)BlendMode.One);
            IntProperty(ownedMaterial, "_DstBlend", (int)BlendMode.Zero);
            IntProperty(ownedMaterial, "_ZWrite", 1);
            IntProperty(ownedMaterial, "_ZTest", (int)CompareFunction.LessEqual);
            IntProperty(ownedMaterial, "_Cull", (int)CullMode.Off);
            ColorProperty(ownedMaterial, "_Color", Color.white);
            ColorProperty(ownedMaterial, "_BaseColor", Color.white);
            ownedMaterial.SetOverrideTag("RenderType", "Opaque");
            ownedMaterial.renderQueue = 2000;
            ownedMaterial.SetShaderPassEnabled("ShadowCaster", false);
            ownedMaterial.enableInstancing = false;
            materialMode = 2;
            return true;
        }

        private static bool CreateMesh()
        {
            donor = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            if (donor == null) return false;
            // Unity creates primitives active. No frame/physics/update boundary
            // occurs between creation and this immediate disable/removal chain.
            donor.SetActive(false);
            donor.hideFlags = HideFlags.HideAndDontSave;
            Renderer renderer = donor.GetComponent<Renderer>();
            if (renderer == null) return false;
            renderer.enabled = false;
            // PhysicsModule is absent from the frozen compile metadata. Read
            // exact engine property on our own SphereCollider only, cold path.
            Component collider = donor.GetComponent("SphereCollider");
            if (collider == null) return false;
            Type colliderType = collider.GetType();
            if (colliderType.FullName != "UnityEngine.SphereCollider") return false;
            PropertyInfo enabled = colliderType.GetProperty("enabled", BindingFlags.Public | BindingFlags.Instance);
            if (enabled == null || enabled.PropertyType != typeof(bool) || !enabled.CanWrite) return false;
            enabled.SetValue(collider, false, null);
            UnityEngine.Object.DestroyImmediate(collider);
            if (donor.GetComponent("SphereCollider") != null) return false;
            MeshFilter filter = donor.GetComponent<MeshFilter>();
            if (filter == null || filter.sharedMesh == null) return false;
            ownedMesh = UnityEngine.Object.Instantiate(filter.sharedMesh) as Mesh;
            if (ownedMesh == null) return false;
            ownedMesh.hideFlags = HideFlags.HideAndDontSave;
            Vector3 extents = ownedMesh.bounds.extents;
            primitiveRadius = Mathf.Max(extents.x, Mathf.Max(extents.y, extents.z));
            UnityEngine.Object.DestroyImmediate(donor);
            donor = null;
            return Finite(primitiveRadius) && primitiveRadius > 0f;
        }

        private static void Clear()
        {
            ClearContact();
            // Destroy only resources allocated/instantiated by this class.
            if (donor != null) { donor.SetActive(false); UnityEngine.Object.Destroy(donor); }
            if (ownedMesh != null) UnityEngine.Object.Destroy(ownedMesh);
            if (ownedMaterial != null) UnityEngine.Object.Destroy(ownedMaterial);
            donor = null; ownedMesh = null; ownedMaterial = null;
#if GF2VR_DIRTY_CONTACT
            prepared = null;
#endif
            preparedSource = null;
            for (int i = 0; i < BuildContract.EyeCapacity; ++i)
            { leftEyes[i] = null; rightEyes[i] = null; lastPair[i] = -1; }
            ownerEpoch = 0; ownerGeneration = 0;
            materialMode = 0; primitiveRadius = 0f;
        }

        public static void Prepare(Camera source, long epoch, long generation, ref int status)
        {
            status = 0;
            if (!OnOwner()) { status = -1; return; }
            bool diagnostics = false;
            try
            {
                if (source != null && source == preparedSource && epoch == ownerEpoch && generation == ownerGeneration &&
                    source.gameObject.activeInHierarchy && ownedMesh != null && ownedMaterial != null)
                {
                    RetryContact(source);
                    status = materialMode == 2 ? -6 : 1;
                    return;
                }
                Clear();
                BeginShaderDiagnostics();
                diagnostics = true;
                if (source == null || !source.gameObject.activeInHierarchy || epoch <= 0 || generation <= 0) return;
                ProbeShaderCandidates();
                if (!CreateMesh()) { Clear(); status = -3; return; }
                if (!CreateMaterial()) { Clear(); status = -5; return; }
                preparedSource = source;
                ownerEpoch = epoch; ownerGeneration = generation;
                drawLayer = source.gameObject.layer;
                RetryContact(source);
                status = materialMode == 2 ? -6 : 1;
            }
            catch (Exception) { Clear(); status = -2; }
            finally { if (diagnostics) FinishShaderDiagnostics(status); }
        }

        private static void Queue(Vector3 position, float scale, Camera left, Camera right)
        {
            Vector3 size;
            size.x = scale; size.y = scale; size.z = scale;
            Matrix4x4 matrix = Matrix4x4.TRS(position, Quaternion.identity, size);
            Graphics.DrawMesh(ownedMesh, matrix, ownedMaterial, drawLayer, left, 0, null,
                ShadowCastingMode.Off, false, null, LightProbeUsage.Off, null);
            Graphics.DrawMesh(ownedMesh, matrix, ownedMaterial, drawLayer, right, 0, null,
                ShadowCastingMode.Off, false, null, LightProbeUsage.Off, null);
        }

        public static void Draw(Camera source, Camera left, Camera right, Vector3 leftPosition,
            Vector3 rightPosition, float radius, float unitScale, int validMask, long epoch, long generation,
            long frame, ref int status)
        {
            status = 0;
            if (!OnOwner()) { status = -1; return; }
            // Hot path: no allocation, object search, reflection, strings or I/O.
            try
            {
                if (preparedSource == null) { status = -40; return; }
                if (ownedMesh == null || ownedMaterial == null) { status = -41; return; }
                if (source == null || source != preparedSource) { status = -42; return; }
                if (epoch != ownerEpoch) { status = -43; return; }
                if (generation != ownerGeneration) { status = -44; return; }
                if (frame < 0) { status = -45; return; }
                if (!source.gameObject.activeInHierarchy) return;
                if ((validMask & 3) == 0)
                {
                    CacheContactHands(leftPosition, rightPosition, radius, unitScale, 0, epoch, generation, frame);
                    return;
                }
                if (left == null || right == null || left == right || left == source || right == source)
                { status = -46; return; }
                int pair = -1, vacant = -1;
                for (int i = 0; i < BuildContract.EyeCapacity; ++i)
                {
                    if (leftEyes[i] == left && rightEyes[i] == right) { pair = i; break; }
                    if (vacant < 0 && leftEyes[i] == null && rightEyes[i] == null) vacant = i;
                }
                if (pair < 0)
                {
                    if (vacant < 0) { status = -47; return; }
                    pair = vacant; leftEyes[pair] = left; rightEyes[pair] = right; lastPair[pair] = -1;
                }
                if (!Finite(radius) || radius <= 0f || radius > 100000f ||
                    !Finite(unitScale) || unitScale < .00001f || unitScale > 100000f ||
                    ((validMask & 1) != 0 && !Finite(leftPosition)) ||
                    ((validMask & 2) != 0 && !Finite(rightPosition))) { status = -7; return; }
                int bit = 1 << drawLayer;
                if ((left.cullingMask & bit) == 0 || (right.cullingMask & bit) == 0)
                { status = -8; return; }
                if (frame <= lastPair[pair]) return;
                float scale = radius / primitiveRadius * (materialMode == 2 ? 0.65f : 1f);
                if (!Finite(scale) || scale <= 0f) { status = -7; return; }
                if ((validMask & 1) != 0) Queue(leftPosition, scale, left, right);
                if ((validMask & 2) != 0) Queue(rightPosition, scale, left, right);
                lastPair[pair] = frame;
                CacheContactHands(leftPosition, rightPosition, radius, unitScale, validMask, epoch, generation, frame);
                status = materialMode == 2 ? -6 : 1;
            }
            catch (Exception) { status = -9; }
        }

        public static void Reset(ref int status)
        {
            status = 0;
            if (!OnOwner()) { status = -1; return; }
            try { Clear(); status = 1; }
            catch (Exception) { status = -2; }
        }

        // All remaining methods are cold F11 diagnostics only.
        private static void Quoted(StringBuilder text, string value)
        {
            if (value == null) { text.Append("null"); return; }
            const string hex = "0123456789abcdef";
            text.Append('"');
            for (int i = 0; i < value.Length; ++i)
            {
                int c = value[i];
                if (c >= 0x20 && c <= 0x7e && c != '"' && c != '\\') text.Append((char)c);
                else
                {
                    text.Append("\\u"); text.Append(hex[(c >> 12) & 15]); text.Append(hex[(c >> 8) & 15]);
                    text.Append(hex[(c >> 4) & 15]); text.Append(hex[c & 15]);
                }
            }
            text.Append('"');
        }

        private static void ObjectValue(StringBuilder text, UnityEngine.Object value)
        {
            if (value == null) { text.Append("null"); return; }
            text.Append("{\"id\":").Append(value.GetInstanceID()).Append(",\"name\":");
            Quoted(text, value.name); text.Append('}');
        }

        private static void Path(StringBuilder text, Transform value)
        {
            text.Append('[');
            Transform[] chain = new Transform[64];
            int count = 0;
            while (value != null && count < chain.Length) { chain[count++] = value; value = value.parent; }
            for (int i = count - 1; i >= 0; --i)
            { if (i != count - 1) text.Append(','); ObjectValue(text, chain[i]); }
            text.Append(']');
        }

#if GF2VR_DIRTY_CONTACT
        private static int BoneId(Transform bone, Dictionary<Transform, int> map, List<Transform> bones)
        {
            if (bone == null) return 0;
            int id;
            if (!map.TryGetValue(bone, out id))
            {
                if (bones.Count >= 32768) return 0;
                id = bone.GetInstanceID(); map.Add(bone, id); bones.Add(bone);
            }
            return id;
        }

        private static void Actor(StringBuilder text, Animator actor)
        {
            text.Append("{\"animator\":"); ObjectValue(text, actor);
            text.Append(",\"root\":"); ObjectValue(text, actor.transform);
            text.Append(",\"path\":"); Path(text, actor.transform);
            text.Append(",\"enabled\":").Append(actor.enabled ? "true" : "false");
            text.Append(",\"active\":").Append(actor.gameObject.activeInHierarchy ? "true" : "false");
            text.Append(",\"update_mode\":").Append((int)actor.updateMode);
            text.Append(",\"culling_mode\":").Append((int)actor.cullingMode);
            text.Append(",\"is_initialized\":").Append(actor.isInitialized ? "true" : "false");
            // Exactly once for each deduplicated Animator root. includeInactive
            // discovers existing disabled components but never activates them.
            DynamicBone[] chains = actor.GetComponentsInChildren<DynamicBone>(true);
            text.Append(",\"dynamicbone_include_inactive_count\":").Append(chains == null ? 0 : chains.Length);
            text.Append(",\"dynamicbones\":[");
            int count = chains == null ? 0 : Math.Min(chains.Length, 512);
            for (int i = 0; i < count; ++i)
            {
                if (i != 0) text.Append(',');
                DynamicBone chain = chains[i];
                if (chain == null) { text.Append("null"); continue; }
                text.Append("{\"component\":"); ObjectValue(text, chain);
                text.Append(",\"enabled\":").Append(chain.enabled ? "true" : "false");
                text.Append(",\"active\":").Append(chain.gameObject.activeInHierarchy ? "true" : "false");
                text.Append(",\"root\":"); ObjectValue(text, chain.m_Root);
                text.Append('}');
            }
            text.Append("],\"dynamicbone_recorded\":").Append(count).Append('}');
        }

        public static string Snapshot(Camera source, int context)
        {
            if (!OnOwner()) return "{\"event\":\"touch_p2_inventory\",\"error\":\"wrong_thread\"}";
            try
            {
                Context actual = Select(source);
                StringBuilder text = new StringBuilder(1024 * 1024);
                int pid = 0;
                try { using (Process process = Process.GetCurrentProcess()) pid = process.Id; }
                catch (Exception) { }
                text.Append("{\"event\":\"touch_p2_inventory\",\"pid\":").Append(pid);
                text.Append(",\"manual_context\":").Append(context);
                text.Append(",\"manual_context_is_classifier\":false,\"helper_context\":").Append(actual == null ? 0 : actual.Kind);
                text.Append(",\"source_exact_owner_camera_match\":").Append(actual != null ? "true" : "false");
                text.Append(",\"unity_frame\":").Append(Time.frameCount).Append(",\"source\":"); ObjectValue(text, source);
                text.Append(",\"material_mode\":").Append(materialMode);
                text.Append(",\"display_owner_path\":\"admitted-source\",\"contact_owner_path\":\"typed-owner\"");
                text.Append(",\"display_prepared_source_match\":").Append(source != null && source == preparedSource ? "true" : "false");
                text.Append(",\"display_epoch\":").Append(ownerEpoch).Append(",\"display_generation\":").Append(ownerGeneration);
                text.Append(",\"contact_select_kind\":").Append(selectKind).Append(",\"contact_select_reason\":"); Quoted(text, selectReason);
                text.Append(",\"contact_main_reject\":"); Quoted(text, mainReject);
                text.Append(",\"contact_dorm_reject\":"); Quoted(text, dormReject);
                text.Append(",\"contact_activity_reject\":"); Quoted(text, activityReject);
                text.Append(",\"shader_diagnostics\":").Append(lastDiagnostics);
                text.Append(",\"contact_runtime\":").Append(ContactDiagnostics());
                text.Append(",\"display_context_rejected\":").Append(source == null || source != preparedSource ? "true" : "false");
                text.Append(",\"owner_root\":"); ObjectValue(text, actual == null ? null : actual.Root);
                text.Append(",\"owner_root_path\":"); Path(text, actual == null ? null : actual.Root.transform);
                if (actual != null && actual.Activity != null)
                    text.Append(",\"activity_is_hall_observed_only\":").Append(actual.Activity.isHallScene ? "true" : "false");
                // Same passive active-object query as P1, once on F11. Actor
                // roots outside the display helper (e.g. Commander) remain in
                // coverage; global presence does not imply display eligibility.
                UnityEngine.Object[] renderers = UnityEngine.Object.FindObjectsOfType(typeof(SkinnedMeshRenderer));
                int total = renderers == null ? 0 : renderers.Length;
                int recorded = Math.Min(total, 512);
                Dictionary<Transform, int> boneMap = new Dictionary<Transform, int>();
                List<Transform> bones = new List<Transform>();
                Dictionary<Transform, int> actorMap = new Dictionary<Transform, int>();
                List<Animator> actors = new List<Animator>();
                int unknownActors = 0, boneLimits = 0;
                text.Append(",\"skinned_renderer_total\":").Append(total);
                text.Append(",\"skinned_renderer_recorded\":").Append(recorded);
                text.Append(",\"renderer_limit\":").Append(total > recorded ? "true" : "false");
                text.Append(",\"renderers\":[");
                for (int i = 0; i < recorded; ++i)
                {
                    if (i != 0) text.Append(',');
                    SkinnedMeshRenderer renderer = renderers[i] as SkinnedMeshRenderer;
                    if (renderer == null) { text.Append("null"); continue; }
                    text.Append("{\"renderer\":"); ObjectValue(text, renderer);
                    text.Append(",\"path\":"); Path(text, renderer.transform);
                    text.Append(",\"enabled\":").Append(renderer.enabled ? "true" : "false");
                    text.Append(",\"active\":").Append(renderer.gameObject.activeInHierarchy ? "true" : "false");
                    bool withinOwner = actual != null && renderer.transform.IsChildOf(actual.Root.transform);
                    text.Append(",\"within_display_helper\":").Append(withinOwner ? "true" : "false");
                    Animator animator = renderer.GetComponentInParent<Animator>();
                    int actorId = 0;
                    if (animator != null)
                    {
                        Transform actorRoot = animator.transform;
                        if (!actorMap.TryGetValue(actorRoot, out actorId))
                        { actorId = actorRoot.GetInstanceID(); actorMap.Add(actorRoot, actorId); actors.Add(animator); }
                    }
                    else ++unknownActors;
                    text.Append(",\"actor_root_id\":").Append(actorId);
                    text.Append(",\"actor_unknown\":").Append(animator == null ? "true" : "false");
                    text.Append(",\"root_bone_id\":").Append(BoneId(renderer.rootBone, boneMap, bones));
                    Transform[] rendererBones = renderer.bones;
                    int boneTotal = rendererBones == null ? 0 : rendererBones.Length;
                    int boneCount = Math.Min(boneTotal, 2048);
                    if (boneCount != boneTotal) ++boneLimits;
                    text.Append(",\"bone_total\":").Append(boneTotal).Append(",\"bone_recorded\":").Append(boneCount);
                    text.Append(",\"bone_ids\":[");
                    for (int j = 0; j < boneCount; ++j)
                    { if (j != 0) text.Append(','); text.Append(BoneId(rendererBones[j], boneMap, bones)); }
                    text.Append("],\"mesh\":"); Mesh mesh = renderer.sharedMesh; ObjectValue(text, mesh);
                    int shapes = mesh == null ? 0 : mesh.blendShapeCount;
                    int shapeCount = Math.Min(shapes, 4096);
                    text.Append(",\"blendshape_total\":").Append(shapes).Append(",\"blendshape_recorded\":").Append(shapeCount);
                    text.Append(",\"blendshapes\":[");
                    for (int j = 0; j < shapeCount; ++j)
                    { if (j != 0) text.Append(','); Quoted(text, mesh.GetBlendShapeName(j)); }
                    text.Append("]}");
                }
                text.Append("],\"bones\":[");
                for (int i = 0; i < bones.Count; ++i)
                {
                    if (i != 0) text.Append(',');
                    Transform bone = bones[i];
                    text.Append("{\"bone\":"); ObjectValue(text, bone);
                    text.Append(",\"path\":"); Path(text, bone);
                    text.Append(",\"parent\":"); ObjectValue(text, bone == null ? null : bone.parent);
                    text.Append('}');
                }
                text.Append("],\"actors\":[");
                for (int i = 0; i < actors.Count; ++i)
                { if (i != 0) text.Append(','); Actor(text, actors[i]); }
                text.Append("],\"actor_unknown_renderer_count\":").Append(unknownActors);
                text.Append(",\"bone_renderer_limits\":").Append(boneLimits);
                text.Append(",\"bone_unique_limit\":").Append(bones.Count >= 32768 ? "true" : "false");
                text.Append(",\"bone_identity_semantics_accepted\":false,\"animation_overwrite_proved\":false}");
                BoneReadProbe.Begin(bones, source, context);
                return text.ToString();
            }
            catch (Exception)
            { return "{\"event\":\"touch_p2_inventory\",\"error\":\"snapshot_exception\",\"engine_writes\":0}"; }
        }
#else
        public static string Snapshot(Camera source, int context)
        {
            if (!OnOwner()) return "{\"event\":\"controller_hands_inventory\",\"error\":\"wrong_thread\"}";
            StringBuilder text = new StringBuilder(512);
            text.Append("{\"event\":\"controller_hands_inventory\",\"flavor\":\"Clean\",\"source\":");
            ObjectValue(text, source);
            text.Append(",\"display_prepared_source_match\":").Append(source != null && source == preparedSource ? "true" : "false");
            text.Append(",\"display_epoch\":").Append(ownerEpoch).Append(",\"display_generation\":").Append(ownerGeneration);
            text.Append(",\"material_mode\":").Append(materialMode).Append(",\"shader_diagnostics\":").Append(lastDiagnostics).Append('}');
            return text.ToString();
        }
#endif
    }
}
#endif

