namespace TreeViewer
{
	partial class FindScript
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
			this.ScriptId = new System.Windows.Forms.TextBox ();
			this.ScriptIdLabel = new System.Windows.Forms.Label ();
			this.FindNext = new System.Windows.Forms.Button ();
			this.Done = new System.Windows.Forms.Button ();
			this.SuspendLayout ();
			// 
			// ScriptId
			// 
			this.ScriptId.Location = new System.Drawing.Point (90, 17);
			this.ScriptId.Margin = new System.Windows.Forms.Padding (2, 2, 2, 2);
			this.ScriptId.Name = "ScriptId";
			this.ScriptId.Size = new System.Drawing.Size (111, 20);
			this.ScriptId.TabIndex = 0;
			this.ScriptId.KeyDown += new System.Windows.Forms.KeyEventHandler (this.ScriptId_KeyDown);
			// 
			// ScriptIdLabel
			// 
			this.ScriptIdLabel.AutoSize = true;
			this.ScriptIdLabel.Location = new System.Drawing.Point (17, 21);
			this.ScriptIdLabel.Margin = new System.Windows.Forms.Padding (2, 0, 2, 0);
			this.ScriptIdLabel.Name = "ScriptIdLabel";
			this.ScriptIdLabel.Size = new System.Drawing.Size (51, 13);
			this.ScriptIdLabel.TabIndex = 1;
			this.ScriptIdLabel.Text = "Script id: ";
			// 
			// FindNext
			// 
			this.FindNext.Location = new System.Drawing.Point (20, 64);
			this.FindNext.Margin = new System.Windows.Forms.Padding (2, 2, 2, 2);
			this.FindNext.Name = "FindNext";
			this.FindNext.Size = new System.Drawing.Size (68, 20);
			this.FindNext.TabIndex = 2;
			this.FindNext.Text = "Find Next";
			this.FindNext.UseVisualStyleBackColor = true;
			this.FindNext.Click += new System.EventHandler (this.FindNext_Click);
			// 
			// Done
			// 
			this.Done.Location = new System.Drawing.Point (133, 64);
			this.Done.Margin = new System.Windows.Forms.Padding (2, 2, 2, 2);
			this.Done.Name = "Done";
			this.Done.Size = new System.Drawing.Size (68, 20);
			this.Done.TabIndex = 3;
			this.Done.Text = "Done";
			this.Done.UseVisualStyleBackColor = true;
			this.Done.Click += new System.EventHandler (this.Done_Click);
			// 
			// FindScript
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF (6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size (220, 107);
			this.Controls.Add (this.Done);
			this.Controls.Add (this.FindNext);
			this.Controls.Add (this.ScriptIdLabel);
			this.Controls.Add (this.ScriptId);
			this.Margin = new System.Windows.Forms.Padding (2, 2, 2, 2);
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "FindScript";
			this.Text = "Find Script";
			this.ResumeLayout (false);
			this.PerformLayout ();

		}

		#endregion

		private System.Windows.Forms.TextBox ScriptId;
		private System.Windows.Forms.Label ScriptIdLabel;
		private System.Windows.Forms.Button FindNext;
		private System.Windows.Forms.Button Done;
	}
}