namespace PDFCilingiri;

partial class Form1
{
    private System.ComponentModel.IContainer components = null;
    private Microsoft.Web.WebView2.WinForms.WebView2 webView;

    protected override void Dispose(bool disposing)
    {
        if (disposing && (components != null))
        {
            components.Dispose();
        }
        base.Dispose(disposing);
    }

    private void InitializeComponent()
    {
        webView = new Microsoft.Web.WebView2.WinForms.WebView2();
        ((System.ComponentModel.ISupportInitialize)webView).BeginInit();
        SuspendLayout();

        // 
        // webView
        // 
        webView.Dock = DockStyle.Fill;
        webView.Location = new Point(0, 0);
        webView.Name = "webView";
        webView.Size = new Size(1264, 811);
        webView.TabIndex = 0;
        webView.DefaultBackgroundColor = Color.FromArgb(15, 23, 42);

        // 
        // Form1
        // 
        AutoScaleDimensions = new SizeF(7F, 15F);
        AutoScaleMode = AutoScaleMode.Font;
        BackColor = Color.FromArgb(15, 23, 42);
        ClientSize = new Size(1264, 811);
        Controls.Add(webView);
        MinimumSize = new Size(860, 600);
        Name = "Form1";
        StartPosition = FormStartPosition.CenterScreen;
        Text = "PDF Çilingiri - Evrak & Belge Stüdyosu";

        ((System.ComponentModel.ISupportInitialize)webView).EndInit();
        ResumeLayout(false);
    }
}
