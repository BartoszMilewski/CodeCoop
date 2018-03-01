using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using Microsoft.Win32;

namespace TreeViewer
{
	public partial class MainWindow : Form
	{
		private ImageList _scriptNodeStateImages;

		public MainWindow ()
		{
			InitializeComponent ();
			_membershipTree.ImageIndex = 0;	// Default
			_membershipTree.SelectedImageIndex = 1;	// Arrow

			_scriptNodeStateImages = new ImageList ();
			_scriptNodeStateImages.Images.Add (SystemIcons.Asterisk);	// 0
			_scriptNodeStateImages.Images.Add (SystemIcons.Exclamation);// 1
			_scriptNodeStateImages.Images.Add (SystemIcons.Error);		// 2
			_membershipTree.StateImageList = _scriptNodeStateImages;
		}

		#region Program menu items

		private void OpenDiagnosticFile_Click (object sender, EventArgs e)
		{
			OpenFileDialog openDlg = new OpenFileDialog ();
			openDlg.InitialDirectory = GetRecentFolder ();
			openDlg.Filter = "Text file|*.txt";
			if (openDlg.ShowDialog () != DialogResult.OK)
				return;

			List<string> memberTable = new List<string> ();
			_membershipTree.BeginUpdate ();
			try
			{
				_membershipTree.Nodes.Clear ();
				// Create an instance of StreamReader to read from a file.
				// The using statement also closes the StreamReader.
				using (StreamReader diagFile = new StreamReader (openDlg.FileName))
				{
					this.Text = openDlg.FileName;
					SetRecentFolder (openDlg.FileName);
					string line;
					// Read the membership
					while ((line = diagFile.ReadLine ()) != null)
					{
						if (line.StartsWith ("===Membership:"))
						{
							ReadMembershipTable (diagFile, memberTable);
							break;
						}
					}

					// Read and display lines from the file until the end of 
					// the file is reached.
					while ((line = diagFile.ReadLine ()) != null)
					{
						while (line.StartsWith ("===Membership changes"))
						{
							string headerDisplayLabel = line.Substring (3);
							int memberIdStartIdx = line.IndexOf (':');
							string memberIdString = line.Substring (memberIdStartIdx + 1);
							int userId = System.Int16.Parse (memberIdString, System.Globalization.NumberStyles.HexNumber);
							headerDisplayLabel += " - ";
							if (userId < memberTable.Count)
								headerDisplayLabel += memberTable [userId];
							else
								headerDisplayLabel += "UNKNOWN MEMBER";
							TreeNode header = _membershipTree.Nodes.Add ("", headerDisplayLabel, 2, 2); // Header nodes don't have keys
							header.Tag = "Header";
							line = ParseMembershipTree (diagFile, header.Nodes);
						}
					}
				}
			}
			catch (Exception ex)
			{
				// Let the user know what went wrong.
				String info;
				info = "The following diagnostic file cannot be read:\n\n";
				info += openDlg.FileName;
				info += "\n\nSystem tells us:\n\n";
				info += ex.Message;
				MessageBox.Show (info);
			}
			_membershipTree.EndUpdate ();
		}

		private void Exit_Click (object sender, EventArgs e)
		{
			Application.Exit ();
		}

		#endregion

		#region Find menu items

		private void FindScript_Click (object sender, EventArgs e)
		{
			Form findScriptWin = new FindScript (_membershipTree);
			findScriptWin.Show (this);

		}

		private void FindDuplicates_Click (object sender, EventArgs e)
		{
			// Finds duplicate scripts in the history
			List<string> scriptsSeenSoFar = new List<string> (_membershipTree.Nodes.Count);
			foreach (TreeNode currentNode in _membershipTree.Nodes)
			{
				string nodeTag = currentNode.Tag as string;
				if (nodeTag == "Header")
					FindDuplicates (currentNode.Nodes, scriptsSeenSoFar);
			}
		}

		private void FindDuplicates (TreeNodeCollection nodes,
									 List<string> scriptsSeenSoFar)
		{
			foreach (TreeNode currentNode in nodes)
			{
				string nodeTag = currentNode.Tag as string;
				if (nodeTag == "Branch")
				{
					FindDuplicates (currentNode.Nodes, scriptsSeenSoFar);
				}
				else if (scriptsSeenSoFar.Contains (nodeTag))
				{
					_membershipTree.SelectedNode = currentNode;
					currentNode.StateImageIndex = 2;
				}
				else
				{
					scriptsSeenSoFar.Add (nodeTag);
				}
			}
		}

		#endregion

		#region Reading member table

		private void ReadMembershipTable (StreamReader diagFile, List<string> memberTable)
		{
			string line;
			while ((line = diagFile.ReadLine ()) != null)
			{
				if (line.StartsWith ("| "))
				{
					// Collect member table row
					int userIdStart = line.IndexOf ('|', 0);
					int userIdStop = line.IndexOf ('|', 1);
					string userIdString = line.Substring (userIdStart + 1, userIdStop - userIdStart - 1);
					int userId = System.Int16.Parse (userIdString, System.Globalization.NumberStyles.HexNumber);
					while (userId > memberTable.Count)
						memberTable.Add ("UNKNOWN MEMBER");

					int nameStart = line.IndexOf ('|', 1);
					int nameStop = line.IndexOf (';', nameStart);
					string memberName = line.Substring (nameStart + 1, nameStop - nameStart - 1);
					memberTable.Add (memberName);
				}
				else if (line.StartsWith ("===Scripts"))
				{
					break;
				}
			}
		}

		#endregion

		#region Parsing diagnostic file and displaying membership tree

		public class ScriptDumpLine
		{
			public ScriptDumpLine (string line)
			{
				int closingBracketIdx = line.IndexOf (']');
				this._scriptId = line.Substring (3, closingBracketIdx - 3);
				int openParentIdx = line.IndexOf ('(');
				int closingParentIdx = line.IndexOf (')');
				this._predecessorId = line.Substring (openParentIdx + 1, closingParentIdx - openParentIdx - 1);
				this._line = line.Substring (closingParentIdx + 1);
			}

			public string ScriptId
			{
				get { return _scriptId; }
			}

			public string PredecessorId
			{
				get { return _predecessorId; }
			}

			public string Line
			{
				get { return _line; }
			}

			public bool IsBranchPoint () { return _line.Contains ("branch point"); }
			public bool IsRejected () { return _line.Contains ("rejected"); }
			public bool IsMissing () { return _line.Contains ("missing"); }

			private string _scriptId;
			private string _predecessorId;
			private string _line;
		};

		private string ParseMembershipTree (StreamReader diagFile, TreeNodeCollection parentNode)
		{
			List<ScriptDumpLine> scriptLines = new List<ScriptDumpLine> ();
			string line;
			while ((line = diagFile.ReadLine ()) != null)
			{
				if (line.StartsWith ("* ["))
				{
					// Collect membership script dump lines
					ScriptDumpLine scriptLine = new ScriptDumpLine (line);
					scriptLines.Add (scriptLine);
				}
				else if (line.StartsWith ("===Membership changes"))
				{
					DisplayMembershipTree (parentNode, scriptLines);
					// End of this membership tree
					return line;
				}
			}
			if (scriptLines.Count != 0)
			{
				// Display last membership tree
				DisplayMembershipTree (parentNode, scriptLines);
			}
			return "";
		}

		private void AddScriptNode (TreeNodeCollection parentNode, ScriptDumpLine scriptLine)
		{
			TreeNode newNode = parentNode.Add (scriptLine.ScriptId,	// Key
											   scriptLine.ScriptId + scriptLine.Line);	// Display label
			newNode.Tag = scriptLine.ScriptId;
			if (scriptLine.IsMissing ())
				newNode.StateImageIndex = 1;
			else if (scriptLine.IsRejected ())
				newNode.StateImageIndex = 0;
		}

		private TreeNode AddBranchNode (TreeNodeCollection parentNode, string scriptId)
		{
			TreeNode branchNode = parentNode.Add ("Branch starting from " + scriptId);
			branchNode.Tag = "Branch";
			return branchNode;
		}

		private void DisplayMembershipTree (TreeNodeCollection parentNode, List<ScriptDumpLine> scriptLines)
		{
			for (int i = scriptLines.Count - 1; i >= 0; --i)
			{
				AddScriptNode (parentNode, scriptLines [i]);
				if (scriptLines [i].IsBranchPoint ())
				{
					TreeNode branchNode = AddBranchNode (parentNode, scriptLines [i].ScriptId);
					i = DisplayBranch (branchNode.Nodes, i - 1, scriptLines);
				}
			}
		}

		private int DisplayBranch (TreeNodeCollection parentNode, int startScriptLine, List<ScriptDumpLine> scriptLines)
		{
			for (int i = startScriptLine; i >= 0; --i)
			{
				AddScriptNode (parentNode, scriptLines [i]);
				if (scriptLines [i].IsBranchPoint ())
				{
					TreeNode branchNode = AddBranchNode (parentNode, scriptLines [i].ScriptId);
					i = DisplayBranch (branchNode.Nodes, i - 1, scriptLines);
				}
				else if (i - 1 >= 0)
				{
					if (scriptLines [i - 1].PredecessorId != scriptLines [i].ScriptId)
						return i;	// Done with this branch
				}
			}
			return -1;
		}

		#endregion

		#region Helpers

		private string _keyName = "HKEY_CURRENT_USER\\Software\\Reliable Software\\Code Co-op\\Preferences\\TreeViewer";

		private string GetRecentFolder ()
		{
			object folder = Registry.GetValue (_keyName, "Recent Folder", null);
			if (folder != null)
				return (string)folder;
			else
				return Environment.GetFolderPath (Environment.SpecialFolder.Desktop);
		}

		private void SetRecentFolder (string filePath)
		{
			string folder = Path.GetDirectoryName (filePath);
			if (folder != string.Empty)
				Registry.SetValue (_keyName, "Recent Folder", folder);
		}

		#endregion
	}
}
