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
            //这里直接在宿主上调用，实际上这里应该使用注入的方式执行的
            HookJIT();

            Application.Run(new Form1());
        }
    }
}