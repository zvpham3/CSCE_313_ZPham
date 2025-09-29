/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Zachary Pham
	UIN: 236002219
	Date: 9/23/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {

	vector<FIFORequestChannel*> channels;

	int opt;
	int p = -1;
	double t = -1.0;
	int e = -1;
	int m  = MAX_MESSAGE;
	bool new_chan = false;
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				new_chan = true;
				break;
			case 'f':
				filename = optarg;
				break;
		}
	}

	string m_str = to_string(m);
	char* serverCMD[] = {(char*) "./server", (char*) "-m", (char*) m_str.data(), nullptr};

	pid_t pid =  fork();
	if(pid == -1)
	{
		cerr << "fork failed\n";
        return 1;
	}

	if(pid == 0)
	{
		execvp(serverCMD[0], serverCMD);
		return 0;
	}

    FIFORequestChannel controlChan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&controlChan);

	if(new_chan)
	{
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		controlChan.cwrite(&nc, sizeof(MESSAGE_TYPE));
		char channel_name[100];
		controlChan.cread(channel_name, sizeof(channel_name));
		FIFORequestChannel* newChan = new FIFORequestChannel(channel_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(newChan);
	}

	FIFORequestChannel* chan =  channels.back();
	// single datapoint
	if(p != -1 && t != -1.0 && e != -1)
	{
		char* buf = new char[m]; // 256
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan->cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan->cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << 
		" is " << reply << endl;	
		delete[] buf;
	}
	else if (p != -1)
	{
		double time = 0;
		char* buf = new char[m]; // 256
		ofstream file("received/x1.csv");
		
		if (!file.is_open()) {
			cerr << "Error opening file!" << endl;
			return 1;
    	}

		for(int i = 0; i < 1000; i++)
		{
			datamsg x(p, time, 1);
			memcpy(buf, &x, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg)); // question
			double reply1;
			chan->cread(&reply1, sizeof(double)); //answer

			datamsg y(p, time, 2);
			memcpy(buf, &y, sizeof(datamsg));
			chan->cwrite(buf, sizeof(datamsg)); // question
			double reply2;
			chan->cread(&reply2, sizeof(double)); //answer

			file << time << "," << reply1 << "," << reply2 << "\n";
			time += 0.004;
		}
		file.close();
		delete[] buf;
	}

    // sending a non-sense message, you need to change this
	if(filename != "")
	{
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan->cwrite(buf2, len);  // I want the file length;
		int64_t filesize = 0;
		chan->cread(&filesize, sizeof(int64_t));


		ofstream file( "received/" + fname);
		if (!file.is_open()) {
			cerr << " test Error opening file!" << endl;
			return 1;
		}

		char* buf3 =  new char[m];
		int64_t offset = 0;
		int length;
		int runtime = 0;
		while(offset != filesize)
		{
			runtime++;
			if(offset + m > filesize)
			{
				length = filesize - offset;
			}
			else
			{
				length = m;
			}

			filemsg* file_req =  (filemsg*) buf2;
			file_req->offset = offset;
			file_req->length = length;
			chan->cwrite(buf2, len);
			chan->cread(buf3, sizeof(char) * length);
			file.write(buf3, length);
			offset += length;
		}
		file.close();
		delete[] buf2;
		delete[] buf3;
	}
	

	if(new_chan)
	{
		MESSAGE_TYPE message = QUIT_MSG;
		chan->cwrite(&message, sizeof(MESSAGE_TYPE));
		channels.pop_back();
		chan = channels.back();
	}
	// closing the channel    
    MESSAGE_TYPE message = QUIT_MSG;
    chan->cwrite(&message, sizeof(MESSAGE_TYPE));
}
