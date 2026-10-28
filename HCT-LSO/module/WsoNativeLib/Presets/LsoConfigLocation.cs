using System;
using System.IO;

namespace WsoNativeLib.Presets
{
    // The one answer to "where does this instrument keep its device configuration".
    //
    // The Toolkit CREATES presets and the Agent CONSUMES them, so both processes have to land
    // on the same folder without either one hardcoding the other's install path.
    //
    // Resolution order, first hit wins:
    //   1. an explicit override (appsettings key / command line) - a bench setup or a test,
    //   2. HCT_LSO_CONFIG_DIR,
    //   3. %ProgramData%\Huvitz\HCT-LSO, but ONLY when it already exists,
    //   4. the calling app's own folder - the behaviour every existing installation has.
    //
    // Rule 3 is opt-in by existence on purpose: until somebody creates that folder nothing
    // changes for an installed Toolkit, and the moment it exists both apps share one set of
    // files without either being reconfigured.
    public static class LsoConfigLocation
    {
        public const string EnvironmentVariableName = "HCT_LSO_CONFIG_DIR";
        public const string DeviceConfigFileName = "WsoDeviceCfg.ini";

        // %ProgramData%\Huvitz\HCT-LSO
        public static string SharedRoot
        {
            get
            {
                string programData = Environment.GetFolderPath(
                    Environment.SpecialFolder.CommonApplicationData);
                return programData.Length == 0
                    ? string.Empty
                    : Path.Combine(programData, "Huvitz", "HCT-LSO");
            }
        }

        // `override` is whatever the app was configured with; null, empty or whitespace means
        // "not configured", which must NOT collapse to the process working directory.
        //
        // A relative override is relative to the APP folder, never Environment.CurrentDirectory:
        // Visual Studio, Explorer and a service can launch the same executable with three
        // different working directories. AppContext.BaseDirectory is stable for all three.
        public static string Resolve(string? overrideDirectory)
        {
            if (!string.IsNullOrWhiteSpace(overrideDirectory))
            {
                return Path.GetFullPath(overrideDirectory!.Trim(), AppContext.BaseDirectory);
            }

            string fromEnvironment = Environment.GetEnvironmentVariable(EnvironmentVariableName)
                ?? string.Empty;
            if (!string.IsNullOrWhiteSpace(fromEnvironment))
            {
                return Path.GetFullPath(fromEnvironment.Trim());
            }

            string shared = SharedRoot;
            if (shared.Length > 0 && Directory.Exists(shared))
            {
                return Path.GetFullPath(shared);
            }

            return Path.GetFullPath(AppContext.BaseDirectory);
        }

        public static string DeviceConfigPath(string? overrideDirectory = null)
        {
            return Path.Combine(Resolve(overrideDirectory), DeviceConfigFileName);
        }

        public static string PresetsDirectory(string? overrideDirectory = null)
        {
            return Path.Combine(Resolve(overrideDirectory), LsoModePresetStore.DirectoryName);
        }

        public const string ReflectedLightDirectoryName = "ReflectedLight";

        public static string ReflectedLightDirectory(string? overrideDirectory = null)
        {
            return Path.Combine(Resolve(overrideDirectory), ReflectedLightDirectoryName);
        }
    }
}
