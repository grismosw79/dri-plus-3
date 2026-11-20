// Build/test API contract only. Never install this assembly in Frosty.
// Identities, signatures and enum value checked against the released RC3 plugin.
using System.Reflection;
[assembly: AssemblyVersion("1.0.0.0")]
namespace FrostySdk.Interfaces { public interface ILogger {} }
namespace FrostySdk {
    public enum ProfileVersion { CollegeFootball27 = 20260605 }
    public static class ProfilesLibrary {
        public static bool Enabled = true;
        public static bool IsLoaded(params ProfileVersion[] versions) {
            return Enabled && versions.Length == 1 && versions[0] == ProfileVersion.CollegeFootball27;
        }
    }
}
