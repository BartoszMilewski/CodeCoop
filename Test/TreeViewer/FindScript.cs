using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace TreeViewer
{
	public partial class FindScript : Form
	{
		private System.Windows.Forms.TreeView _scripts;
		private System.Windows.Forms.TreeNode [] _foundScripts;
		private int _currentFoundScript;
		private string _currentScriptId;

		public FindScript (TreeView scripts)
		{
			InitializeComponent ();

			this._scripts = scripts;
			_foundScripts = new TreeNode [0];
			_currentFoundScript = -1;
			_currentScriptId = "";
		}

		private void FindNext_Click (object sender, EventArgs e)
		{
			StartFind ();
		}

		private void Done_Click (object sender, EventArgs e)
		{
			this.Close ();
		}

		private void ScriptId_KeyDown (object sender, KeyEventArgs e)
		{
			if (e.KeyCode == Keys.Enter)
				StartFind ();
		}

		private void StartFind ()
		{
			if (ScriptId.Text != _currentScriptId)
			{
				_currentScriptId = ScriptId.Text;
				_foundScripts = _scripts.Nodes.Find (_currentScriptId, true);	// Search all children
				_currentFoundScript = -1;
			}

			if (_foundScripts.GetLength (0) == 0)
				return;

			_currentFoundScript = (_currentFoundScript + 1) % _foundScripts.GetLength (0);
			TreeNode scriptNode = _foundScripts [_currentFoundScript];
			if (_scripts.SelectedNode != scriptNode)
				_scripts.SelectedNode = scriptNode;
		}
	}
}