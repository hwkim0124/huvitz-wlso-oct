using System;
using System.Windows;

namespace WsoToolkit
{
    public partial class LightControlWindow : Window
    {
        public LightControlWindow()
        {
            InitializeComponent();
        }

        // The owning screen's capture/preview one-call drives the same LEDs, so the values are
        // re-read whenever this window comes back to the front - not just when it is created.
        private void Window_Activated(object sender, EventArgs e)
        {
            myLightPanel.Refresh();
        }
    }
}
