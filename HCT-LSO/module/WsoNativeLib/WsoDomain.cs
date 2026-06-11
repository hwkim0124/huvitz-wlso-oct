


namespace WsoNativeLib
{
    public static class WsoDomain
    {
        public enum EyeSide : int
        {
            UNKNOWN = 0,
            OD = 1, 
            OS = 2, 
            BOTH = 3
        }

        public enum EyeRegion : int
        {
            UNKNOWN = 0,
            MACULAR = 1,
            OPTIC_DISC = 2,
            FUNDUS = 3,
            CORNEA = 4, 
            MIRROR = 5,
        }

        public enum FixationTarget : int
        {
            UNKNOWN = 0,
            CENTER = 1,
            OPTIC_DISC = 2,
            FUNDUS = 3,
            LEFT_SIDE = 4,
            LEFT_TOP = 5,
            LEFT_BOTTOM = 6,
            RIGHT_SIDE = 7,
            RIGHT_TOP = 8,
            RIGHT_BOTTOM = 9
        }

        public enum OcularLayerType : int
        {
            UNKNOWN = 0,
            ILM = 101,
            NFL = 102,
            IPL = 103,
            OPL = 104,
            IOS = 105,
            RPE = 106,
            OPR = 107,
            BRM = 108,
            BMO = 109,      //baseline for disc BMO Imgae
            EPI = 201,
            BOW = 202,
            END = 203,
            INNER = 1,
            OUTER = 2
        }

    }

}