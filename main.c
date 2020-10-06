#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <termios.h>

#define PORT 9008

char rbuf[64];

void send_cmd(int uart_fd, char *buf, int send_len, int recv_len)
{
	int read_len;
	
	read_len = write(uart_fd, buf, send_len);
	printf("Write response == %d \n",read_len);


	read_len = read(uart_fd, rbuf, recv_len);	
	printf("Respose from Sensor(%d) : ", read_len);
	
	for(int j=0; j< read_len; j++)
	{
		printf(" %02x ",rbuf[j]);
	}

	printf("\n");

}

int calc_two_hex(int idx)
{
	int high, low;

	high = rbuf[idx] * 16* 16;
	low = rbuf[idx+1];

	return (high + low);

}

void main()
{
	struct sockaddr_in serveraddr, server_addr, client_addr;
	int client_sockfd, server_sock = 0;
	int client_len, addr_len, ret;
	int client_sock = 0;

	printf("Trying to open serial....\n");

	
	int xfd  = open("/dev/i2c-1" , O_RDWR | O_NOCTTY);

	
	struct termios xbeeio;
	
	printf("Serial description is %d....\n",xfd);
	
	float f_temp;

	char laser_ctrl[] = {0x50,0x16, 0x07, 0x11, 0x02, 0x08, 0x04, 0xe1};
	char open_particle[] = {0x50,0x16, 0x08, 0x11, 0x03, 0x0c, 0x02, 0x1e, 0xc0};
	char conti_mode[] = {0x50, 0x16, 0x08,  0x11, 0x03, 0x0d, 0xff, 0xff, 0xe1};
	char single_mode[] = {0x50, 0x16, 0x08, 0x11, 0x03, 0x0d, 0x00, 0x24, 0xbb};
	char get_data[] = {0x50, 0x16, 0x07,  0x11, 0x02, 0x01, 0x01, 0xeb};	
	
	
	int cnt = 0;

	if ((server_sock =  socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("ERROR: fail to creat server sock %d\n", server_sock);
	}
	else
	{
		memset(&server_addr, 0x00, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(PORT);

		if(bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr))<0)
		{
			printf("Socket binding error \n");
		}
		else
		{
			if(listen(server_sock, 5) < 0)
			{
				printf("Socket listening error \n");
			}
		}

	}

	addr_len = sizeof(client_addr);
	
	
      	
	printf("Single  mode command send\n ");
	
	send_cmd(xfd, conti_mode, 8, 7);
	send_cmd(xfd,laser_ctrl, 7, 6);
	
	printf(" Open Particle command sent\n");
	
	send_cmd(xfd, open_particle, 8, 6);
	sleep(1);
	
	

	printf("*** Request of sensor data array ***\n");
	
	for(;;)
	{
		send_cmd(xfd, get_data, 7, 26);
		
		printf("CO2 level = %d ppm\n",calc_two_hex(4)); 	
		printf("VOC level = %d Level\n",calc_two_hex(6));
		printf("Humidity  = %2.1f %\n", (float)calc_two_hex(8)/10); 	
		printf("Temperature = %2.1f C\n", ((float)calc_two_hex(10)-500)/10); 	
		printf("PM 1.0(GRIMM) = %d ug/m3\n",calc_two_hex(12));
		printf("PM 2.5(GRIMM) = %d ug/m3\n",calc_two_hex(14));
		printf("PM 10(GRIMM) = %d ug/m3\n",calc_two_hex(16));


		printf("Waiting for Client Connection....\n");
		

		
		client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_len);
	
		if(client_sock >0)
		{
			printf("Client ip %s  \n", inet_ntoa(client_addr.sin_addr));
		
			ret = send(client_sock, rbuf, 26,0);
		

			if(ret<1)
			{

		
				printf("Client is disconnected !!! \n");
			}
		}else
		{
			printf("Client is not connected !!! \n");
		}


		close(client_sock);
		
		sleep(3);

	}


	close(xfd);
	close(client_sockfd);
	
}

