/**
 * @file   ThreadRxUDP.cs
 * @author B. Beaulieu
 * @date   oct 2022
 * @brief  Classe thread pour la réception UDP. Utilisé par la Démo Serveur UDP du cour 516
 * 
 * Compilateur: VS 2015
 */
 

using System.Text;

using System.Net; //ajout manuel
using System.Net.Sockets; //ajout manuel
using System.Windows.Forms;
using System.Collections.Specialized;
using System.Collections.Generic;
using Lab4;

namespace DemoUDP
{
    class ThreadRxUDP
    {
        const int PORT_RX = 2223;  //Port de réception UDP
        const int MAX_TRAME = 512; //Grosseur max du buffer de réception

        private byte[] m_trameRx = new byte[MAX_TRAME];  //buffer de Rx


        private IPAddress ipClient;  //à titre d'info pour savoir qui a émit la trame UDP
        private int portClient; //idem pour le port

        private UdpClient udpClient;

        private Form ptrMain;

        public delegate void monProtoDelegate(List<byte> trame, string s);
        public monProtoDelegate objDelegate;

        /// <summary>
        /// Constructeur
        /// </summary>
        public ThreadRxUDP(Form ptF1)
        {
            ptrMain = ptF1;

            udpClient = new UdpClient(PORT_RX);
        }

    


        /// <summary>
        /// Méthode principale appelée par le Thread
        /// </summary>
        public void FaitTravail()
        {
            List<byte> lstTrameRx = new List<byte>();
            IPEndPoint IpDistant = new IPEndPoint(IPAddress.Any, 0);

            //le thread tourne toujours dans cette boucle en attente d'une trame UDP
            while (true)
            {
                m_trameRx = udpClient.Receive(ref IpDistant);
                ipClient = ((IPEndPoint)IpDistant).Address;  //ip du client UDP qui a émit la trame
                portClient = ((IPEndPoint)IpDistant).Port;   //port du client UDP qui a émit la trame

                if(m_trameRx.Length == (int)Form1.enumTrame.maxTrame)
                {
                    for(int i = 0; i < m_trameRx.Length; i++)
                    {
                        lstTrameRx.Add(m_trameRx[i]);
                    }
                    if(Form1.verifTrame(lstTrameRx))
                        ptrMain.BeginInvoke(objDelegate, lstTrameRx, IpDistant.ToString());
                }
                
            }
        }

        /// <summary>
        /// Construit une string avec l'ip, le port et le message reçu.
        /// </summary>
        /// <returns>L'adr IP, le port et la trame reçue formatés en String </returns>
        public string GetMsgRecu()
        {
            if (ipClient != null)
                return ipClient.ToString() + " (" + portClient.ToString() + "):   " + Encoding.Default.GetString(m_trameRx);// Encoding.ASCII.GetString(m_trameRx);
            else
                return "";
        }


        public void ArreteClientUDP()
        {
            udpClient.Close();
        }

    }
}
