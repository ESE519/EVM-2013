    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.Data;
    using System.Drawing;
    using System.Linq;
    using System.Text;
    using System.Windows.Forms;
    using System.IO.Ports;
    using System.Threading;

    namespace ESE519
    {
        public partial class Form1 : Form
        {
           public String readString;
            SerialPort comport; //= new SerialPort("COM19", 9600, Parity.None, 8, StopBits.One);
            char[] buff = new char[5];
            char[] inputArray = new char[1024];

             enum COMMANDS
            {
                SEND_COMMAND = 1,
                SLAVE_STATES = 2,
                FUNCTION_NAMES = 3,
                TRANSMISSION_STATES = 4
             };
            
             ImageList nodeImage = new ImageList();



            //TextBox textbox1 = new TextBox();
            public Form1()
            {
                InitializeComponent();
            
                string[] ports = SerialPort.GetPortNames();
                foreach(string port in ports)
                {
                    comboBox1.Items.Add(port);
                }

                initializeImage();

            
            

                //printThread.Start();
            }

            private void Form1_Load(object sender, EventArgs e)
            {
                InitUIComponents();
            }

            void InitUIComponents()
            {
                //button2.Enabled = false;
                textBox1.Enabled = false;
            
            
            
                //Disable UI elements 
           
 
                


            }
            void initializeImage()
            {
            nodeImage.Images.Add(Image.FromFile(@"C:\Users\sumukhn\Desktop\RED.png"));
            nodeImage.Images.Add(Image.FromFile(@"C:\Users\sumukhn\Desktop\GREEN.png"));
            nodeImage.Images.Add(Image.FromFile(@"C:\Users\sumukhn\Desktop\ORANGE.png"));
            pictureBox1.Image = nodeImage.Images[0];
            pictureBox2.Image = nodeImage.Images[0];
            }

            private void textBox1_TextChanged(object sender, EventArgs e)
            {

            }

            void updateFunctionNames(char[] buffer1)
            {
             
                if (buffer1[1] == 1)
                {
                    char[] stringbuff = new char[buffer1.Length];
                    int j=0;
                    for(int i=0;i<buffer1.Length-2;i++)
                    {
                        stringbuff[i] = buffer1[2+j];
                        Console.WriteLine(stringbuff[i].ToString());
                        j++;
                        
                    }
                    String hope = new String(stringbuff);
                    Console.WriteLine(hope);
                    this.Invoke((MethodInvoker)delegate
                    {
                        Node1Tasks.Text = hope;
                    });
                    Console.WriteLine("HAHA");
                }
            
             
                if (buffer1[1] == 2)
                {
                    char[] stringbuff = new char[buffer1.Length+1];
                    int j=0;
                    int i = 0;
                    for( i=0;i<buffer1.Length-2;i++)
                    {
                        stringbuff[i] = buffer1[2+j];
                        
                           
                        j++;

                    }
                    String hope = new String(stringbuff);
                    Console.WriteLine(hope);
                    this.Invoke((MethodInvoker)delegate
                    {
                        Node2Tasks.Text = hope;
                    });
                    Console.WriteLine("NAHA");
                }


            }

            void updateNeighborList(char[] buffer1)
            {

                if (buffer1[1] == '1')
                {
                    this.Invoke((MethodInvoker)delegate
                    {
                        pictureBox1.Image = nodeImage.Images[1];
                    });
                    Console.WriteLine("Node 1 Alive");
                }
                else
                {
                    this.Invoke((MethodInvoker)delegate
                    {
                        pictureBox1.Image = nodeImage.Images[0];
                    });
                    Console.WriteLine("Node 1 Dead");


                }
                if (buffer1[2] == '1')
                {
                    this.Invoke((MethodInvoker)delegate
                    {
                        pictureBox2.Image = nodeImage.Images[1];
                    });
                    Console.WriteLine("Node 2 Alive");
                }
                else
                {
                    this.Invoke((MethodInvoker)delegate
                    {
                        pictureBox2.Image = nodeImage.Images[0];
                    });

                    Console.WriteLine("Node 2 Dead");
                }
           

            }
            void updateTranmissionStates(char[] buffer1)
            {
                String str1, str2;
                if (buffer1.Length > 2)
                {
                    if (buffer1[1] == 2)
                    {

                        switch (buffer1[2])
                        {
                            case (char)0:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "IDLE";


                                });
                                break;
                            case (char)1:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "TASK PARAMETERS TRANSMISSION";


                                });



                                break;
                            case (char)2:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "FUNCTION NAMES TRANSMISSION";


                                });



                                break;

                            case (char)3:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "FUNCTION CODE TRANSMISSION";


                                });

                                break;
                            case (char)4:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "INITIATING TASK";


                                });

                                break;

                            case (char)5:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "DEACTIVATE TASK";


                                });

                                break;

                            case (char)6:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "ACTIVATE TASK";


                                });

                                break;

                            case (char)7:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node1Tasks.Text = "SENDING BLOCKS OF CODE";


                                });

                                break;




                        }






                    }
                    else
                    {
                        this.Invoke((MethodInvoker)delegate
                        {
                            Node1Tasks.Text = "IDLE";


                        });
                    }
                    if (buffer1[1] == 3)
                    {

                        switch (buffer1[2])
                        {
                            case (char)0:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "IDLE";


                                });
                                break;
                            case (char)1:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "TASK PARAMETERS TRANSMISSION";


                                });



                                break;
                            case (char)2:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "FUNCTION NAMES TRANSMISSION";


                                });



                                break;

                            case (char)3:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "FUNCTION CODE TRANSMISSION";


                                });

                                break;
                            case (char)4:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "SENDING INITIAL TASK";


                                });

                                break;

                            case (char)5:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "DEACTIVATE TASK";


                                });

                                break;

                            case (char)6:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "ACTIVATE TASK";


                                });

                                break;

                            case (char)7:
                                this.Invoke((MethodInvoker)delegate
                                {
                                    Node2Tasks.Text = "SENDING BLOCKS OF CODE";


                                });

                                break;


                        }
                    }
                    else
                    {

                        this.Invoke((MethodInvoker)delegate
                        {
                            Node2Tasks.Text = "IDLE";


                        });

                    }


                }
                else
                {
                    this.Invoke((MethodInvoker)delegate
                    {
                        Node2Tasks.Text = "IDLE";
                        Node1Tasks.Text = "IDLE";

                    });
                }
                    
                  
                    






            }
            void com_DataReceived(object sender, SerialDataReceivedEventArgs e)
            {
       

                try
                {

                    SerialPort sp = (SerialPort)sender;

                    //MessageBox.Show("Entered Here");
                    char [] buffer = new char[sp.BytesToRead];
                    int readBytes = sp.Read(buffer, 0, buffer.Length);
                   // MessageBox.Show(buffer.ToString());
                    //label1.Text = buffer.ToString();
                    //readString= sp.ReadLine();
              
                    //inputArray = readString.ToCharArray();
                    //Console.WriteLine(readString);
                    for (int j = 0; j < readBytes; j++)
                    {
                        //Console.WriteLine(buffer[j].ToString());
                    }
                    if (buffer[0] == 3)
                    {
                        Console.WriteLine("HAHAH");


                    }
                    
                    switch(buffer[0])
                    {
                        case (char)COMMANDS.SEND_COMMAND:
                            break;
                             
                        case (char)COMMANDS.SLAVE_STATES:
                        updateNeighborList(buffer);
                        
                        break;
                        

                        case (char)COMMANDS.FUNCTION_NAMES:
                        updateFunctionNames(buffer);
                        Console.WriteLine("Function Name Received");
                        break;
                        
                        case (char)COMMANDS.TRANSMISSION_STATES:
                        updateTranmissionStates(buffer);
                        Console.WriteLine("Transmission States");
                        break;

                        
                        default:
                            String printStr = new String(buffer);
                      
                            this.Invoke((MethodInvoker)delegate
                        {
                            textBox1.AppendText(printStr);
                            textBox1.AppendText("\n");
                        });
                         
                        break;
                    }
                   



                  
                    
                 
                     
                }
                catch (Exception ex)
                {

                }
            }

            private void label1_Click(object sender, EventArgs e)
            {

            }

            private void button1_Click_1(object sender, EventArgs e)
            {
                Object selectedItem = comboBox1.SelectedItem;
                int selectedIndex = comboBox1.SelectedIndex;

                
                comport = new SerialPort(selectedItem.ToString(), 9600, Parity.None, 8, StopBits.One);
                comport.DataReceived += new SerialDataReceivedEventHandler(com_DataReceived);
                try
                {
                    comport.Open();
                }
                catch (Exception ex)
                {
                }
                comboBox1.Enabled = false;
                button1.Enabled = false;


                //Enable the UI elements 

            

          
                
                //button2.Enabled = true;
                textBox1.Enabled = true;


            }

    
            private void pictureBox5_Click(object sender, EventArgs e)
            {

            }

            private void label2_Click(object sender, EventArgs e)
            {

            }

            private void pictureBox1_Click(object sender, EventArgs e)
            {

            }

            private void pictureBox2_Click(object sender, EventArgs e)
            {

            }

            private void pictureBox3_Click(object sender, EventArgs e)
            {

            }

            private void pictureBox4_Click(object sender, EventArgs e)
            {
                
            }

            private void label3_Click(object sender, EventArgs e)
            {

            }

            private void tableLayoutPanel1_Paint(object sender, PaintEventArgs e)
            {

            }

            private void button2_Click_1(object sender, EventArgs e)
            {
                buff[0] = (char)0x01;
                buff[1] = '\r';
                buff[2] = '\n';
                comport.Write(buff, 0, 3);
                button2.Enabled = false;
            }

            private void button3_Click(object sender, EventArgs e)
            {
                buff[0] = (char)0x02;
                buff[1] = '\r';
                buff[2] = '\n';
                comport.Write(buff, 0, 3);
                button3.Enabled = false;

            }

            private void button4_Click(object sender, EventArgs e)
            {
                buff[0] = (char)0x03;
                buff[1] = '\r';
                buff[2] = '\n';
                comport.Write(buff, 0, 3);
                button4.Enabled = false;

            }
 
        }
    }