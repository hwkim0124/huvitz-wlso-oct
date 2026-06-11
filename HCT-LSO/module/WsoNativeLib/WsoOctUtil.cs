using Microsoft.Windows.Themes;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WsoNativeLib
{
    using static WsoDomain;
    using static WsoOctDefs;
    using static WsoOctScan;

    public static class WsoOctUtil
    {
        public static bool IsMacularScan(OctPatternCode code)
        {
            return ((int)code >= 1000 && (int)code < 2000);
        }

        public static bool IsMacularScan(in OctProtocolDescript desc)
        {
            return IsMacularScan(desc.patternCode);
        }

        public static bool IsOpticDiscScan(OctPatternCode code)
        {
            return ((int)code >= 2000 && (int)code < 3000);
        }

        public static bool IsOpticDiscScan(in OctProtocolDescript desc)
        {
            return IsOpticDiscScan(desc.patternCode);
        }

        public static bool IsCorneaScan(OctPatternCode code)
        {
            return ((int)code >= 4000 && (int)code < 5000);
        }

        public static bool IsCorneaScan(in OctProtocolDescript desc)
        {
            return IsCorneaScan(desc.patternCode);
        }

        public static string GetEyeSideString(WsoDomain.EyeSide side)
        {
            /*
            return side switch
            {
                WsoDomain.EyeSide.OD => "OD",
                WsoDomain.EyeSide.OS => "OS",
                _ => "Unknown",
            };
            */
            switch (side)
            {
                case WsoDomain.EyeSide.OD:
                    return "OD";
                case WsoDomain.EyeSide.OS:
                    return "OS";
                default:
                    return "Unknown";
            }
        }

        public static string GetEyeRegionString(WsoDomain.EyeRegion region)
        {
            /*
            return region switch
            {
                WsoDomain.EyeRegion.MIRROR => "Mirror",
                WsoDomain.EyeRegion.MACULAR => "Macular",
                WsoDomain.EyeRegion.OPTIC_DISC => "Optic Disc",
                WsoDomain.EyeRegion.FUNDUS => "Fundus",
                WsoDomain.EyeRegion.CORNEA => "Cornea",
                _ => "Unknown",
            };
            */
            switch (region)
            {
                case WsoDomain.EyeRegion.MIRROR:
                    return "Mirror";
                case WsoDomain.EyeRegion.MACULAR:
                    return "Macular";
                case WsoDomain.EyeRegion.OPTIC_DISC:
                    return "Optic Disc";
                case WsoDomain.EyeRegion.FUNDUS:
                    return "Fundus";
                case WsoDomain.EyeRegion.CORNEA:
                    return "Cornea";
                default:
                    return "Unknown";
            }
        }

        public static string GetScanSpeedString(OctScanSpeed speed)
        {
            /*
            return speed switch
            {
                OctScanSpeed.SLOWER => "Slower",
                OctScanSpeed.NORMAL => "Normal",
                OctScanSpeed.FASTER => "Faster",
                OctScanSpeed.CUSTOM => "Custom",
                _ => "Unknown",
            };
            */
            switch (speed)
            {
                case OctScanSpeed.SLOWER:
                    return "Slower";
                case OctScanSpeed.NORMAL:
                    return "Normal";
                case OctScanSpeed.FASTER:
                    return "Faster";
                case OctScanSpeed.CUSTOM:
                    return "Custom";
                default:
                    return "Unknown";
            }
        }

        public static string GetScanDirectionString(OctScanDirection direction)
        {
            /*
            return direction switch
            {
                OctScanDirection.X_TO_Y => "X-Y",
                OctScanDirection.Y_TO_X => "Y-X", 
                _ => "Unknown"
            };
            */
            switch (direction)
            {
                case OctScanDirection.X_TO_Y: 
                    return "X-Y";
                case OctScanDirection.Y_TO_X:
                    return "Y-X";
                default:
                    return "Unknown";
            }
        }

        public static OcularLayerType LayerStringToType(string text)
        {
            OcularLayerType type;
            if (Enum.TryParse(text, out type))
            {
                return type;
            }
            else
            {
                return OcularLayerType.UNKNOWN;
            }
        }

        public static string LayerTypeToString(OcularLayerType type)
        {
            return Enum.GetName(typeof(OcularLayerType), type) ?? "UNKNOWN";
        }

        public static bool IsStratumParamValid(OctStratumParam param)
        {
            if (param.upperType == OcularLayerType.UNKNOWN || param.lowerType == OcularLayerType.UNKNOWN)
            {
                return false;
            }
            if ((int)param.upperType > (int)param.lowerType)
            {
                return false;
            }
            if (((int)param.upperType == (int)param.lowerType) && (param.upperOffset >= param.lowerOffset))
            {
                return false;
            }
            return true;
        }

        public static int[] GetLayerPoints(OcularLayerType type, ref WsoOctSegm.OctRetinaBsegmDescriptor segm)
        {
            switch (type)
            {
                case OcularLayerType.ILM:
                    return segm.ilms;
                case OcularLayerType.NFL:
                    return segm.nfls;
                case OcularLayerType.IPL:
                    return segm.ipls;
                case OcularLayerType.OPL:
                    return segm.opls;
                case OcularLayerType.IOS:
                    return segm.ioss;
                case OcularLayerType.RPE:
                    return segm.rpes;
                case OcularLayerType.BRM:
                    return segm.brms;
                case OcularLayerType.OPR:
                    return segm.oprs;
                case OcularLayerType.BMO:
                    return segm.datums;
                case OcularLayerType.INNER:
                    return segm.inners;
                case OcularLayerType.OUTER:
                    return segm.outers;
                default:
                    return new int[0];
            }
        }

        public static int[] GetLayerPoints(OcularLayerType type, ref WsoOctSegm.OctCorneaBsegmDescriptor segm)
        {
            switch (type)
            {
                case OcularLayerType.EPI:
                    return segm.epis;
                case OcularLayerType.BOW:
                    return segm.bows;
                case OcularLayerType.END:
                    return segm.ends;
                case OcularLayerType.BMO:
                    return segm.datums;
                case OcularLayerType.INNER:
                    return segm.inners;
                case OcularLayerType.OUTER:
                    return segm.outers;
                default:
                    return new int[0];
            }
        }
    }
}
