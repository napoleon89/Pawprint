using Antlr4.StringTemplate;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace reflect {
	class Program {
		static bool isInvalidChar(char c) {
			return c == '\r'
				|| c == '\n'
				|| c == ' ';
		}

		static bool isReflectableToken(string token) {
			return token == "struct"
				|| token == "class";
		}

		static string getNextToken(string text, ref int index) {
			int start_index = index;
			while (!isInvalidChar(text[index])) index++;
			int length = index - start_index;
			return text.Substring(start_index, length);
		}

		static void logError(string message) {
			ConsoleColor start_color = Console.ForegroundColor;
			Console.ForegroundColor = ConsoleColor.Red;
			Console.WriteLine("Error: {0}", message);
			Console.ForegroundColor = start_color;
		}

		[Serializable]
		struct ReflectedField {
			public string type_id;
			public string name;
		};

		[Serializable]
		struct ReflectedType {
			public string name;
			public List<ReflectedField> fields;
		}

		static void Main(string[] args) {
			Dictionary<string, string> args_data = new Dictionary<string, string>();
			

			foreach (string arg in args) {
				string arg_key = arg.Substring(0, arg.IndexOf(':')).TrimStart('-');
				string data = arg.Substring(arg_key.Length+3, arg.Length - arg_key.Length-3);
				args_data[arg_key] = data;
			}
			
			if(!args_data.ContainsKey("input")) {
				logError("No input specified");
				return;
			}

			if (!args_data.ContainsKey("template")) {
				logError("No template specified");
				return;
			}

			if (!args_data.ContainsKey("output")) {
				logError("No output specified");
				return;
			}

			bool has_excludes = args_data.ContainsKey("exclude");
			string exclude = "";
			if (has_excludes) exclude = args_data["exclude"];

			bool has_inc_base = args_data.ContainsKey("inc_base");
			string inc_base = "";
			if (has_inc_base) inc_base = args_data["inc_base"];

			Stopwatch timer = Stopwatch.StartNew();
			string reflect_token = "REFLECT_STRUCT";
			string reflect_field_token = "REFLECT_FIELD";

			List<ReflectedType> types = new List<ReflectedType>();
			string[] files = Directory.GetFiles(args_data["input"], "*.h", SearchOption.AllDirectories);

			string working_dir = Directory.GetCurrentDirectory();

			string includes_output = "";

			foreach (string file in files) {
				if (has_excludes && file.Replace(working_dir, "") == exclude) continue;
				string file_contents = File.ReadAllText(file);
				int last_index = 0;
				int start_type_amount = types.Count;
				while ((last_index = file_contents.IndexOf(reflect_token, last_index)) != -1) {
					last_index += reflect_token.Length;
					while (isInvalidChar(file_contents[last_index])) last_index++;
					string next_token = getNextToken(file_contents, ref last_index);
					if (!isReflectableToken(next_token)) {
						logError(string.Format("Token {0} is not a reflectable type", next_token));
					}
					last_index++;
					string reflectable_name = getNextToken(file_contents, ref last_index);

					ReflectedType type = new ReflectedType();
					type.name = reflectable_name;
					type.fields = new List<ReflectedField>();

					last_index++;
					{
						while (file_contents[last_index] != '{')
							last_index++;
						last_index++;
						int struct_start = last_index;

						int scope_count = 1;

						while (true) {
							char current = file_contents[last_index++];
							if (current == '{') scope_count++;
							else if (current == '}') scope_count--;
							if (scope_count == 0) {
								break;
							}
						}

						int struct_length = last_index - struct_start - 1;
						string struct_string = file_contents.Substring(struct_start, struct_length);
						int field_search_index = 0;

						while ((field_search_index = struct_string.IndexOf(reflect_field_token, field_search_index)) != -1) {
							field_search_index += reflect_field_token.Length + 1;
							int start = field_search_index;
							while (struct_string[field_search_index] != ';') field_search_index++;
							int field_length = field_search_index - start;
							//Console.Write("  " + struct_string.Substring(start, field_length));
							int field_name_search_index = start + field_length;
							int field_name_length = 0;
							while (struct_string[field_name_search_index] != ' ') { field_name_search_index--; field_name_length++; }
							field_name_search_index++;
							field_name_length--;
							string field_name = struct_string.Substring(field_name_search_index, field_name_length);
							string field_type = struct_string.Substring(start, field_name_search_index - start - 1);
							ReflectedField field = new ReflectedField() {
								name = field_name,
								type_id = field_type
							};
							type.fields.Add(field);
						}

					}
					types.Add(type);
				}
				if(start_type_amount < types.Count) {
					includes_output += "#include <" + file.Replace(inc_base, "") + ">\r\n";
				}
			}

			Template template = new Template(File.ReadAllText(args_data["template"]), '$', '$');
			string result = "";

			for(int i = 0; i < types.Count; i++) {
				ReflectedType type = types[i];
				template.Add("type", type.name);
				template.Add("field_count", type.fields.Count);
				template.Add("fields", type.fields);
				string output = template.Render();
				template.Remove("type");
				template.Remove("fields");
				template.Remove("field_count");
				result += output;
			}

			string output_file_contents = File.ReadAllText(args_data["output"]);
			string begin_token = "// == REFLECT_DATA_BEGIN";
			int start_data = output_file_contents.IndexOf(begin_token);
			start_data += begin_token.Length;
			output_file_contents = output_file_contents.Remove(start_data, output_file_contents.Length - start_data) + "\r\n";
			output_file_contents += includes_output;
			output_file_contents += "\r\n" + result;
			File.WriteAllText(args_data["output"], output_file_contents);

			ConsoleColor start_color = Console.ForegroundColor;
			Console.ForegroundColor = ConsoleColor.Green;
			Console.WriteLine("Completed reflection in " + timer.Elapsed.TotalMilliseconds + "ms");
			Console.ForegroundColor = start_color;
		}
	}
}
