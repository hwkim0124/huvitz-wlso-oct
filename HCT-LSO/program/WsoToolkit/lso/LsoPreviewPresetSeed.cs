using System;
using System.Collections.Generic;
using System.IO;
using WsoNativeLib.Presets;
using WsoNativeLib.Utils;

namespace WsoToolkit
{
    // Loads the mode presets, and on an instrument that has no usable Preview or Color Capture
    // preset of its own installs the tuned preset shipped next to the app (SeedPresets/, linked
    // from HCT-LSO/config/lso/Presets by WsoToolkit.csproj) and selects it.
    //
    // ponytail: one fixed seed name per role; when the shared preset is re-tuned under a new name,
    // change the name here and the csproj link together.
    public static class LsoPreviewPresetSeed
    {
        private static readonly (ColorCameraPresetMode Mode, string Key, string SeedName)[] Seeds =
        {
            (ColorCameraPresetMode.Preview, "PreviewPreset", "Preview_Bfs_1019"),
            (ColorCameraPresetMode.Capture, "CapturePreset", "Capture_Bfs_1019"),
        };

        public static string SeedDirectory => Path.Combine(AppContext.BaseDirectory, "SeedPresets");

        // Call instead of store.Load(). The result is a line for the operator when a seed was
        // installed (or could not be), null when the instrument already had its own presets.
        public static string? Load(LsoModePresetStore store, string configIniPath)
        {
            // Decided before Load: Load seeds the DevKit defaults and selects them, after which
            // every instrument looks set up.
            var ini = new IniFileUtil(configIniPath, createIfMissing: false);
            var missing = new List<(ColorCameraPresetMode Mode, string SeedName)>();
            foreach (var seed in Seeds)
            {
                string selected = ini.ReadString(LsoModePresetStore.IniSectionName, seed.Key, string.Empty).Trim();
                if (!store.Exists(selected))
                {
                    missing.Add((seed.Mode, seed.SeedName));
                }
            }

            store.Load(); // binds Capture to Color, so Select(Capture) below writes CapturePreset
            if (missing.Count == 0)
            {
                return null;
            }

            if (!Directory.Exists(SeedDirectory))
            {
                return $"No tuned preset was set up and the shipped seed folder '{SeedDirectory}' is missing - "
                    + $"'{store.Preview.Name}' / '{store.Capture.Name}' are used.";
            }

            try
            {
                // Never overwrite: a file of the same name on this instrument is the operator's.
                foreach (string source in Directory.GetFiles(SeedDirectory))
                {
                    string target = Path.Combine(store.Directory, Path.GetFileName(source));
                    if (!File.Exists(target))
                    {
                        File.Copy(source, target);
                    }
                }
            }
            catch (Exception e) when (e is IOException || e is UnauthorizedAccessException)
            {
                return $"The shipped tuned presets could not be installed: {e.Message} - "
                    + $"'{store.Preview.Name}' / '{store.Capture.Name}' are used.";
            }

            var lines = new List<string>();
            foreach (var (mode, seedName) in missing)
            {
                lines.Add(store.Select(mode, seedName)
                    ? $"No {mode} preset was set up on this instrument - installed and selected the shipped '{seedName}'."
                    : $"The shipped {mode} preset '{seedName}' could not be selected - '{store.Active(mode).Name}' is used.");
            }
            return string.Join(Environment.NewLine, lines);
        }
    }
}
