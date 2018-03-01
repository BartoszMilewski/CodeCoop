namespace TreeViewer
{
	partial class MainWindow
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose (bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose ();
			}
			base.Dispose (disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent ()
		{
			this.components = new System.ComponentModel.Container ();
			System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager (typeof (MainWindow));
			this.TopMenu = new System.Windows.Forms.MenuStrip ();
			this.Program = new System.Windows.Forms.ToolStripMenuItem ();
			this.OpenDiagnosticFile = new System.Windows.Forms.ToolStripMenuItem ();
			this.Exit = new System.Windows.Forms.ToolStripMenuItem ();
			this.findToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem ();
			this.scriptToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem ();
			this.duplicatesToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem ();
			this._membershipTree = new System.Windows.Forms.TreeView ();
			this._membershipTreeImages = new System.Windows.Forms.ImageList (this.components);
			this.TopMenu.SuspendLayout ();
			this.SuspendLayout ();
			// 
			// TopMenu
			// 
			this.TopMenu.Items.AddRange (new System.Windows.Forms.ToolStripItem [] {
            this.Program,
            this.findToolStripMenuItem});
			this.TopMenu.Location = new System.Drawing.Point (0, 0);
			this.TopMenu.Name = "TopMenu";
			this.TopMenu.Size = new System.Drawing.Size (338, 24);
			this.TopMenu.TabIndex = 0;
			this.TopMenu.Text = "Menu";
			// 
			// Program
			// 
			this.Program.DropDownItems.AddRange (new System.Windows.Forms.ToolStripItem [] {
            this.OpenDiagnosticFile,
            this.Exit});
			this.Program.Name = "Program";
			this.Program.Size = new System.Drawing.Size (59, 20);
			this.Program.Text = "Program";
			// 
			// OpenDiagnosticFile
			// 
			this.OpenDiagnosticFile.Name = "OpenDiagnosticFile";
			this.OpenDiagnosticFile.Size = new System.Drawing.Size (197, 22);
			this.OpenDiagnosticFile.Text = "Open Diagnostic File ...";
			this.OpenDiagnosticFile.Click += new System.EventHandler (this.OpenDiagnosticFile_Click);
			// 
			// Exit
			// 
			this.Exit.Name = "Exit";
			this.Exit.Size = new System.Drawing.Size (197, 22);
			this.Exit.Text = "Exit";
			this.Exit.Click += new System.EventHandler (this.Exit_Click);
			// 
			// findToolStripMenuItem
			// 
			this.findToolStripMenuItem.DropDownItems.AddRange (new System.Windows.Forms.ToolStripItem [] {
            this.scriptToolStripMenuItem,
            this.duplicatesToolStripMenuItem});
			this.findToolStripMenuItem.Name = "findToolStripMenuItem";
			this.findToolStripMenuItem.Size = new System.Drawing.Size (39, 20);
			this.findToolStripMenuItem.Text = "Find";
			// 
			// scriptToolStripMenuItem
			// 
			this.scriptToolStripMenuItem.Name = "scriptToolStripMenuItem";
			this.scriptToolStripMenuItem.Size = new System.Drawing.Size (164, 22);
			this.scriptToolStripMenuItem.Text = "Script ...";
			this.scriptToolStripMenuItem.Click += new System.EventHandler (this.FindScript_Click);
			// 
			// duplicatesToolStripMenuItem
			// 
			this.duplicatesToolStripMenuItem.Name = "duplicatesToolStripMenuItem";
			this.duplicatesToolStripMenuItem.Size = new System.Drawing.Size (164, 22);
			this.duplicatesToolStripMenuItem.Text = "Duplicate Scripts";
			this.duplicatesToolStripMenuItem.Click += new System.EventHandler (this.FindDuplicates_Click);
			// 
			// _membershipTree
			// 
			this._membershipTree.Cursor = System.Windows.Forms.Cursors.Hand;
			this._membershipTree.Dock = System.Windows.Forms.DockStyle.Fill;
			this._membershipTree.FullRowSelect = true;
			this._membershipTree.ImageIndex = 0;
			this._membershipTree.ImageList = this._membershipTreeImages;
			this._membershipTree.Indent = 20;
			this._membershipTree.Location = new System.Drawing.Point (0, 24);
			this._membershipTree.Name = "_membershipTree";
			this._membershipTree.SelectedImageIndex = 1;
			this._membershipTree.Size = new System.Drawing.Size (338, 342);
			this._membershipTree.TabIndex = 1;
			// 
			// _membershipTreeImages
			// 
			this._membershipTreeImages.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject ("_membershipTreeImages.ImageStream")));
			this._membershipTreeImages.TransparentColor = System.Drawing.Color.Transparent;
			this._membershipTreeImages.Images.SetKeyName (0, "");
			this._membershipTreeImages.Images.SetKeyName (1, "");
			this._membershipTreeImages.Images.SetKeyName (2, "");
			// 
			// MainWindow
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF (6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size (338, 366);
			this.Controls.Add (this._membershipTree);
			this.Controls.Add (this.TopMenu);
			this.MainMenuStrip = this.TopMenu;
			this.Name = "MainWindow";
			this.Text = "Code Co-op Diagnostics Tree Viewer";
			this.TopMenu.ResumeLayout (false);
			this.TopMenu.PerformLayout ();
			this.ResumeLayout (false);
			this.PerformLayout ();

		}

		#endregion

		private System.Windows.Forms.MenuStrip TopMenu;
		private System.Windows.Forms.TreeView _membershipTree;
		private System.Windows.Forms.ToolStripMenuItem Program;
		private System.Windows.Forms.ToolStripMenuItem OpenDiagnosticFile;
		private System.Windows.Forms.ToolStripMenuItem Exit;
		private System.Windows.Forms.ToolStripMenuItem findToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem scriptToolStripMenuItem;
		private System.Windows.Forms.ToolStripMenuItem duplicatesToolStripMenuItem;
		private System.Windows.Forms.ImageList _membershipTreeImages;
	}
}

