using System;
using System.Collections.Generic;
using System.Text;

namespace WsoToolkit.entity
{
    internal class ExamDto
    {
        public string EyeSide { get; set; } = "OD";
        public string DateTime { get; set; } = "";
        public string MeasureType { get; set; } = "";
        public string MeasureName { get; set; } = "";

        public bool IsIdentity(ExamDto exam)
        {
            return EyeSide == exam.EyeSide && DateTime == exam.DateTime && MeasureType == exam.MeasureType && MeasureName == exam.MeasureName;
        }

        public static bool IsValidSloExamPath(string path)
        {
            if (path.EndsWith(".jpg") && path.IndexOf("SLO_") >= 0)
            {
                if (path.IndexOf("preview") < 0 && path.Split('_').Length == 5)
                {
                    return true;
                }
            }
            return false;
        }

        public void ParseFromDirectoryName(string directoryName)
        {
            string[] array = directoryName.Split('_');
            if (array.Length == 5)
            {
                MeasureType = array[0];
                DateTime = GetDateTimeFormat(array[1], array[2]);
                EyeSide = array[3];
                MeasureName = array[4];
            }
        }

        private string GetDateTimeFormat(string date, string time)
        {
            string forms = "";
            if (date.Length == 8)
            {
                string text = date.Substring(0, 4);
                string text2 = date.Substring(4, 2);
                string text3 = date.Substring(6, 2);
                forms = text + "-" + text2 + "-" + text3;

                if (time.Length == 6)
                {
                    text = time.Substring(0, 2);
                    text2 = time.Substring(2, 2);
                    text3 = time.Substring(4, 2);
                    forms += (" " + text + ":" + text2 + ":" + text3);
                    return forms;
                }
            }

            forms = date + " " + time;
            return forms;
        }
    }
}
