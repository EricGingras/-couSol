/**
 * @file   frmConfigPort.cs
 * @author Eric Gingras
 * @date   octobre 2022
 * @brief  Class pour la configuration des ports.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace Lab3
{
    public partial class frmConfigPort : Form
    {
        //variables membres de la classe frmConfigPort
        public string m_nom { get; set; }
        public int m_vitesse { get; set; }
        public int m_nbBit { get; set; }
        public Parity m_parite { get; set; }
        public int m_stopBit { get; set; }

        /// <summary>
        /// Constructeur qui reçoit les valeurs actuelles de la configuration 
        /// du port série en paramètres.
        /// </summary>
        /// <param name="nom"></param>
        /// <param name="vitesse"></param>
        /// <param name="nbBit"></param>
        /// <param name="parite"></param>
        /// <param name="stopBit"></param>
        public frmConfigPort(string nom, int vitesse, int nbBit, Parity parite, int stopBit)
        {
            InitializeComponent();

            //Remplissage de tous les combo boxes avec les informations de configuration 
            cbPorts.DataSource = SerialPort.GetPortNames();
            cbVitesse.DataSource = new int[] { 2400, 4800, 9600, 14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000 };
            cbDataBits.DataSource = new int[] { 7, 8 };
            cbStopBits.DataSource = new int[] { 1, 2 };
            cbParity.DataSource = Enum.GetValues(typeof(Parity));

            //Transmission de la configuration actuelle du port
            cbPorts.SelectedItem = nom;
            cbVitesse.SelectedItem = vitesse;
            cbDataBits.SelectedItem = nbBit;
            cbParity.SelectedItem = parite;
            cbStopBits.SelectedItem = stopBit;
        }

        /// <summary>
        /// Assignage de tous les valeurs des ComboBox aux variables 
        /// membres pour que les autres forms peuvent les utilisés
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void btnOk_Click(object sender, EventArgs e)
        {
            //On assigne les valeurs des ComboBox aux variables membres
            if (cbPorts.SelectedIndex >= 0)
                m_nom = cbPorts.SelectedItem.ToString();
            m_vitesse = Convert.ToInt32(cbVitesse.SelectedItem);
            m_nbBit = Convert.ToInt16(cbDataBits.SelectedItem);
            m_parite = (Parity)cbParity.SelectedItem;
            m_stopBit = (int)cbStopBits.SelectedItem;
        }
    }
}
