using System;
using System.Drawing;
using System.IO;
using System.IO.Compression;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;

namespace PDFCilingiri;

public partial class Form1 : Form
{
    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attr, ref int attrValue, int attrSize);

    private const int DWMWA_USE_IMMERSIVE_DARK_MODE = 20;

    public Form1()
    {
        InitializeComponent();
        ApplyDarkTitleBar();
        LoadAppIcon();
        this.Load += Form1_Load;
    }

    private void ApplyDarkTitleBar()
    {
        try
        {
            if (Environment.OSVersion.Version.Major >= 10)
            {
                int darkMode = 1;
                DwmSetWindowAttribute(this.Handle, DWMWA_USE_IMMERSIVE_DARK_MODE, ref darkMode, sizeof(int));
            }
        }
        catch { }
    }

    private void LoadAppIcon()
    {
        try
        {
            string localIcon = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "app.ico");
            if (File.Exists(localIcon))
            {
                this.Icon = new Icon(localIcon);
                return;
            }

            var assembly = Assembly.GetExecutingAssembly();
            using var stream = assembly.GetManifestResourceStream("PDFCilingiri.app.ico");
            if (stream != null)
            {
                this.Icon = new Icon(stream);
            }
        }
        catch { }
    }

    private async void Form1_Load(object? sender, EventArgs e)
    {
        try
        {
            string assetsPath = EnsureAssetsExtracted();

            string userDataFolder = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                "PDFCilingiri",
                "WebView2_Data"
            );
            Directory.CreateDirectory(userDataFolder);

            var env = await CoreWebView2Environment.CreateAsync(null, userDataFolder);
            await webView.EnsureCoreWebView2Async(env);

            webView.CoreWebView2.Settings.IsStatusBarEnabled = false;
            webView.CoreWebView2.Settings.AreDefaultContextMenusEnabled = true;
            webView.CoreWebView2.Settings.AreDevToolsEnabled = true;

            // Map https://pdfcilingiri.local to the assets folder
            webView.CoreWebView2.SetVirtualHostNameToFolderMapping(
                "pdfcilingiri.local",
                assetsPath,
                CoreWebView2HostResourceAccessKind.Allow
            );

            // Handle downloads nicely
            webView.CoreWebView2.DownloadStarting += (s, args) =>
            {
                // Allow default behavior (user can see and save downloaded PDFs/ZIPs)
                args.Handled = false;
            };

            webView.CoreWebView2.Navigate("https://pdfcilingiri.local/index.html");
        }
        catch (Exception ex)
        {
            MessageBox.Show(
                "Uygulama başlatılırken bir hata oluştu:\n\n" + ex.Message + "\n\nEdge WebView2 bileşeninin kurulu olduğundan emin olun.",
                "PDF Çilingiri Hatası",
                MessageBoxButtons.OK,
                MessageBoxIcon.Error
            );
        }
    }

    private static string EnsureAssetsExtracted()
    {
        // 1. Check if an "assets" folder is right next to the .exe (for portable/live editing)
        string baseDir = AppDomain.CurrentDomain.BaseDirectory;
        string directAssets = Path.Combine(baseDir, "assets");
        if (File.Exists(Path.Combine(directAssets, "index.html")))
        {
            return directAssets;
        }

        // 2. Otherwise extract embedded assets.zip to LocalAppData
        string targetDir = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "PDFCilingiri",
            "app"
        );

        try
        {
            Directory.CreateDirectory(targetDir);
            var assembly = Assembly.GetExecutingAssembly();
            
            // Try to find assets.zip stream
            string[] names = assembly.GetManifestResourceNames();
            string? resourceName = null;
            foreach (var name in names)
            {
                if (name.EndsWith("assets.zip", StringComparison.OrdinalIgnoreCase))
                {
                    resourceName = name;
                    break;
                }
            }

            if (resourceName != null)
            {
                using var stream = assembly.GetManifestResourceStream(resourceName);
                if (stream != null)
                {
                    using var archive = new ZipArchive(stream);
                    archive.ExtractToDirectory(targetDir, true);
                }
            }
        }
        catch { }

        return targetDir;
    }
}
