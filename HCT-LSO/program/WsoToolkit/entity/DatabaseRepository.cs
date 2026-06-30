using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Text;

namespace WsoToolkit.entity
{
    internal class DatabaseRepository
    {
        public static ObservableCollection<PatientDto> Patients = new ObservableCollection<PatientDto>();
        public static ObservableCollection<ExamDto> Exams = new ObservableCollection<ExamDto>();

        public static string RootPath = ".\\patients";
        public static string ResultPath = ".\\results";
        public static int CurrentIndex = -1;

        public static void CreateResultDirectory()
        {
            if (!Directory.Exists(ResultPath))
            {
                Directory.CreateDirectory(ResultPath);
            }
        }

        public static void ReconstructRepository()
        {
            Patients.Clear();

            if (!Directory.Exists(RootPath))
            {
                Directory.CreateDirectory(RootPath);
            }

            var sub_dirs = Directory.GetDirectories(RootPath);
            foreach (string pat_dir in sub_dirs)
            {
                try
                {
                    string path = pat_dir + "\\patient.json";
                    if (File.Exists(path))
                    {
                        var json = File.ReadAllText(path);
                        var inst = Newtonsoft.Json.JsonConvert.DeserializeObject<PatientDto>(json);
                        if (inst != null)
                        {
                            Patients.Add(inst);
                        }

                    }
                }
                catch (Exception e)
                {
                    Console.WriteLine(e.Message);
                    continue;
                }
            }
        }

        public static bool FindPatient(PatientDto patient)
        {
            foreach (var pat in Patients)
            {
                if (pat.IsIdentity(patient))
                {
                    return true;
                }
            }
            return false;
        }

        public static bool InsertPatient(PatientDto patient)
        {
            foreach (var pat in Patients)
            {
                if (pat.IsIdentity(patient))
                {
                    return false;
                }
            }

            try
            {
                string inst_dir = RootPath + "\\" + patient.GetDirectoryName();
                string json_path = inst_dir + "\\patient.json";
                if (Directory.Exists(inst_dir))
                {
                    return false;
                }
                var info = Directory.CreateDirectory(inst_dir);
                patient.Path = inst_dir; //  info.FullName;

                using (StreamWriter sw = File.CreateText(json_path))
                {
                    var json = Newtonsoft.Json.JsonConvert.SerializeObject(patient);
                    sw.Write(json);
                }

                Patients.Add(patient);
                CurrentIndex = Patients.Count - 1;
                return true;
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return false;
            }
        }

        public static void UpdateExamList()
        {
            Exams.Clear();

            var patient = GetCurrentPatient();
            if (patient == null || patient.Path.Length == 0)
            {
                return;
            }

            try
            {
                var dir_path = GetCurrentPatientPath();
                var paths = Directory.GetDirectories(dir_path);
                foreach (string path in paths)
                {
                    if (path.IndexOf("OCT_") >= 0)
                    {
                        var exam = new ExamDto();
                        var name = System.IO.Path.GetFileName(path);
                        exam.ParseFromDirectoryName(name);
                        var exist = false;
                        for (int i = 0; i < Exams.Count; i++)
                        {
                            if (Exams[i].IsIdentity(exam))
                            {
                                exist = true;
                                break;
                            }
                        }
                        if (!exist)
                        {
                            Exams.Add(exam);
                        }
                    }
                }

                var files = Directory.GetFiles(dir_path);
                foreach (string path in files)
                {
                    if (ExamDto.IsValidSloExamPath(path))
                    {
                        var exam = new ExamDto();
                        var name = System.IO.Path.GetFileName(path);
                        name = name.Substring(0, name.Length - 4);
                        exam.ParseFromDirectoryName(name);
                        var exist = false;
                        for (int i = 0; i < Exams.Count; i++)
                        {
                            if (Exams[i].IsIdentity(exam))
                            {
                                exist = true;
                                break;
                            }
                        }
                        if (!exist)
                        {
                            Exams.Add(exam);
                        }
                    }
                }
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public static PatientDto? GetCurrentPatient()
        {
            if (Patients.Count == 0 || CurrentIndex < 0)
            {
                return null;
            }
            return Patients[CurrentIndex];
        }

        public static void SetCurrentPatient(int index)
        {
            if (index >= 0 && index < Patients.Count)
            {
                CurrentIndex = index;
            }
        }

        public static string GetCurrentPatientPath()
        {
            var patient = GetCurrentPatient();
            if (patient != null)
            {
                int index = patient.Path.IndexOf("patients");
                if (index > 0)
                {
                    var path = ".\\" + patient.Path.Substring(index);
                    return path;
                }
                return patient.Path;
            }
            return "";
        }

        public static string GetCurrentPatientTag()
        {
            var patient = GetCurrentPatient();
            if (patient != null)
            {
                return patient.GetLabelText();
            }
            return "";
        }
    }
}
