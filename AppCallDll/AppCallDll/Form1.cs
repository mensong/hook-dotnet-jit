using System;
using System.Collections.Generic;
using System.ComponentModel;

using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace AppCallDll
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            //
            MessageBox.Show(doAddFun(123));
        }
        private string doAddFun(int m)
        {
            int n = 50;
            n = n * m;
            return n.ToString();
        }
    }
}