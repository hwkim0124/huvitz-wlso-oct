using System;
using System.Collections.Generic;
using System.Text;

namespace WsoToolkit.entity
{
    internal class PatientDto
    {
        public string Pid { get; set; } = "";
        public string Name { get; set; } = "";
        public string Gender { get; set; } = "Male";
        public string BirthDate { get; set; } = "";
        public string DiopterOD { get; set; } = "";
        public string DiopterOS { get; set; } = "";
        public string Description { get; set; } = "";
        public string Path { get; set; } = "";

        public bool IsValid()
        {
            return !string.IsNullOrEmpty(Pid) && !string.IsNullOrEmpty(Name);
        }

        public bool IsIdentity(PatientDto patient)
        {
            return Pid == patient.Pid;
        }

        public string GetDirectoryName()
        {
            // return Pid + "_" + Name;
            return Pid;
        }

        public int GetGenderType()
        {
            return Gender == "Female" ? 1 : 0;
        }

        public string GetLabelText()
        {
            return "[ " + Pid + " :: " + Name + " / " + Gender + " / " + BirthDate + " ]";
        }
    }
}
