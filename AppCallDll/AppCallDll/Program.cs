using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Runtime.InteropServices;

namespace AppCallDll
{
    static class Program
    {
        [DllImport("HookJitPrj.dll", CallingConvention = CallingConvention.Cdecl)]
        private static extern void HookJIT();

        /// <summary>
        /// 应用程序的主入口点。
        /// </summary>
        
        [STAThread]
        static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            //try
            //{
            HookJIT();
            //}
            //catch { }
            Application.Run(new Form1());
        }
    }
}