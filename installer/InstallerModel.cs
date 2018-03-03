using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Data.OleDb;
using System.IO;
using System.Xml.Serialization;
using Microsoft.VisualBasic.FileIO;

namespace installer
{
   class InstallerModel
   {
      public List<String> SearchForInstallPaths()
      {
         List<String> install_paths = new List<String>();

         using (var connection = new OleDbConnection(@"Provider=Search.CollatorDSO;Extended Properties=""Application=Windows"""))
         {

            var query = "SELECT \"System.ItemUrl\" FROM \"SystemIndex\" WHERE System.Kind = SOME ARRAY['program'] AND CONTAINS(System.FileName, '\"battlefrontii.exe\"')";

            connection.Open();

            using (var command = new OleDbCommand(query, connection))
            {

               using (var reader = command.ExecuteReader())
               {
                  while (reader.Read())
                  {
                     var path = reader[0].ToString();
                     path = path.Substring(5);

                     install_paths.Add(path);
                  }
               }

               connection.Close();

            }
         }

         return install_paths;
      }

      public void Install(string path)
      {
         Directory.CreateDirectory(Path.Combine(path, "data/shaderpatch/backup/"));

         if (!File.Exists(Path.Combine(path, "data/shaderpatch/backup/core.lvl")))
         {
            File.Move(Path.Combine(path, "data/_lvl_pc/core.lvl"),
                      Path.Combine(path, "data/shaderpatch/backup/core.lvl"));
         }

         FileSystem.CopyDirectory("./", path, true);
         
         List<string> files = new List<string>();

         files.Add("./data/shaderpatch/file_manifest.xml");

         foreach (string file in Directory.GetFiles("./", "*", System.IO.SearchOption.AllDirectories))
            files.Add(file);

         XmlSerializer serializer = new XmlSerializer(typeof(List<string>));

         using (var stream = File.OpenWrite(Path.Combine(path, "data/shaderpatch/file_manifest.xml")))
         {
            serializer.Serialize(stream, files);
         }
      }
   }
}
