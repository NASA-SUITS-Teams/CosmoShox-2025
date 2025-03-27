#include "SendUDPMessage.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "Common/UdpSocketReceiver.h"
#include <winsock2.h>
#undef SetPort // Undefine the SetPort macro to avoid conflicts

// Function to convert a 32-bit integer to network byte order (big-endian)
uint32 HostToNetworkOrder(uint32 value)
{
    return htonl(value);
}

// Function to convert a 32-bit float to network byte order (big-endian)
uint32 FloatToNetworkOrder(float value)
{
    uint32 intRepresentation = *reinterpret_cast<uint32*>(&value);
    return HostToNetworkOrder(intRepresentation);
}

void ASendUDPMessage::SendMessageToTSS(int32 TimeStamp, int32 CommandNumber, float InputData, int32 Port, const FString& Address)
{
    // UE_LOG(LogTemp, Warning, TEXT("Input TimeStamp: %d, CommandNumber: %d, InputData: %f"), TimeStamp, CommandNumber, InputData);
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Purple, FString::Printf(TEXT("Input TimeStamp: %d, CommandNumber: %d, InputData: %f"), TimeStamp, CommandNumber, InputData));

    FSocket* Socket = FUdpSocketBuilder(TEXT("MySocket"))
        .AsReusable()
        .WithBroadcast()
        .WithReceiveBufferSize(1024); // Use a larger buffer size

    TSharedRef<FInternetAddr> RemoteAddress = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    bool bIsValid;
    RemoteAddress->SetIp(*Address, bIsValid);
    RemoteAddress->SetPort(Port);

    if (!bIsValid)
    {
        UE_LOG(LogTemp, Warning, TEXT("Invalid IP address: %s"), *Address);
        return;
    }

    // Prepare the data to be sent (two int32 values and one float value in big-endian format)
    TArray<uint8> Data;
    Data.SetNumUninitialized(12);

    uint32 BigEndianTimeStamp = HostToNetworkOrder(static_cast<uint32>(TimeStamp));
    uint32 BigEndianCommandNumber = HostToNetworkOrder(static_cast<uint32>(CommandNumber));
    uint32 BigEndianInputData = FloatToNetworkOrder(InputData);

    FMemory::Memcpy(Data.GetData(), &BigEndianTimeStamp, 4);
    FMemory::Memcpy(Data.GetData() + 4, &BigEndianCommandNumber, 4);
    FMemory::Memcpy(Data.GetData() + 8, &BigEndianInputData, 4);

    // Log the values being sent to ensure they are copied correctly
    uint32* SentTimeStamp = reinterpret_cast<uint32*>(Data.GetData());
    uint32* SentCommandNumber = reinterpret_cast<uint32*>(Data.GetData() + sizeof(uint32));
    uint32* SentInputData = reinterpret_cast<uint32*>(Data.GetData() + 2 * sizeof(uint32));

    // UE_LOG(LogTemp, Warning, TEXT("Sending TimeStamp (big-endian): %u, CommandNumber (big-endian): %u, InputData (big-endian): %u"), *SentTimeStamp, *SentCommandNumber, *SentInputData);
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, FString::Printf(TEXT("Sending TimeStamp (big-endian): %u, CommandNumber (big-endian): %u, InputData (big-endian): %u"), *SentTimeStamp, *SentCommandNumber, *SentInputData));

    // Send the message
    int32 BytesSent = 0;
    bool bSuccess = Socket->SendTo(Data.GetData(), Data.Num(), BytesSent, *RemoteAddress);

    // Log the result of the send operation
    //if (bSuccess)
    //{
    //    // UE_LOG(LogTemp, Warning, TEXT("Successfully sent %d bytes"), BytesSent);
    //    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, FString::Printf(TEXT("Successfully sent %d bytes"), BytesSent));
    //}
    //else
    //{
    //    // UE_LOG(LogTemp, Error, TEXT("Failed to send UDP message"));
    //    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Failed to send UDP message")));
    //}

    // Start receiving the response asynchronously
    StartReceiving(Socket);
}

void ASendUDPMessage::StartReceiving(FSocket* Socket)
{
    // GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Red, FString::Printf(TEXT("Start Receiving Called!")));

    // Create the UDP socket receiver
    FTimespan ThreadWaitTime = FTimespan::FromMilliseconds(100);
    UDPReceiver = MakeShareable(new FUdpSocketReceiver(Socket, ThreadWaitTime, TEXT("MyUDPReceiver")));
    UDPReceiver->OnDataReceived().BindUObject(this, &ASendUDPMessage::OnDataReceived);
    UDPReceiver->Start();
}

void ASendUDPMessage::OnDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Endpoint)
{
    // GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("OnDataReceived Called!")));

    FReceivedData ReceivedDataStruct;
    ReceivedDataStruct.TimeStamp = 0;
    ReceivedDataStruct.CommandNumber = 0;
    ReceivedDataStruct.FloatData = 0.0f;
    ReceivedDataStruct.IntData = 0;
    ReceivedDataStruct.bIsFloat = false;

    // Log the raw data received
    FString RawDataString;
    for (int32 i = 0; i < Data->Num(); i++)
    {
        RawDataString += FString::Printf(TEXT("%02x "), (*Data)[i]);
    }
    // UE_LOG(LogTemp, Warning, TEXT("Raw Data Received: %s"), *RawDataString);
    // GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Red, FString::Printf(TEXT("Raw Data Received: %s"), *RawDataString));

    if (Data->Num() >= sizeof(uint32) * 2) // Ensure we have at least the timestamp and command number
    {
        ReceivedDataStruct.TimeStamp = ntohl(*reinterpret_cast<uint32*>(Data->GetData()));
        ReceivedDataStruct.CommandNumber = ntohl(*reinterpret_cast<uint32*>(Data->GetData() + sizeof(uint32)));

        // UE_LOG(LogTemp, Warning, TEXT("Received TimeStamp: %u, CommandNumber: %u"), ReceivedDataStruct.TimeStamp, ReceivedDataStruct.CommandNumber);
        // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Received TimeStamp: %u, CommandNumber: %u"), ReceivedDataStruct.TimeStamp, ReceivedDataStruct.CommandNumber));

        if (Data->Num() == sizeof(uint32) * 3)
        {
            // Received a single float
            uint32 ReceivedDataRaw = ntohl(*reinterpret_cast<uint32*>(Data->GetData() + 2 * sizeof(uint32)));
            float ReceivedFloatData;
            FMemory::Memcpy(&ReceivedFloatData, &ReceivedDataRaw, sizeof(float));

            // Log the received data
            // UE_LOG(LogTemp, Warning, TEXT("Received Data (raw): %u"), ReceivedDataRaw);
            // GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Received Data (raw): %u"), ReceivedDataRaw));

            // Log the converted data
            // UE_LOG(LogTemp, Warning, TEXT("Converted Float Data: %f"), ReceivedFloatData);
            // GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Purple, FString::Printf(TEXT("Converted Float Data: %f"), ReceivedFloatData));

            // Store the received float data
            ReceivedDataStruct.FloatData = ReceivedFloatData;
            ReceivedDataStruct.bIsFloat = true;
            // UE_LOG(LogTemp, Warning, TEXT("Interpreted as Float Data: %f"), ReceivedDataStruct.FloatData);
            // GEngine->AddOnScreenDebugMessage(-1, 15.f, FColor::Green, FString::Printf(TEXT("Interpreted as Float Data: %f"), ReceivedDataStruct.FloatData));

            // Check if the received data is a boolean
            if (ReceivedFloatData == 1.0f || ReceivedFloatData == 0.0f)
            {
                bool ReceivedBoolData = (ReceivedFloatData == 1.0f);
                // Trigger the event with the boolean value
                OnDataReceivedEvent.Broadcast(ReceivedBoolData ? 1.0f : 0.0f);
            }
            else
            {
                // Trigger the event with the float value
                OnDataReceivedEvent.Broadcast(ReceivedFloatData);
            }
        }
        else if (Data->Num() == sizeof(uint32) * 2 + sizeof(float) * 13)
        {
            // Received LIDAR data (13 floats)
            for (int32 i = 0; i < 13; i++)
            {
                uint32 LidarValueRaw = ntohl(*reinterpret_cast<uint32*>(Data->GetData() + sizeof(uint32) * 2 + sizeof(float) * i));
                float LidarValue;
                FMemory::Memcpy(&LidarValue, &LidarValueRaw, sizeof(float));
                ReceivedDataStruct.LidarData.Add(LidarValue);

                // UE_LOG(LogTemp, Warning, TEXT("Received LIDAR Data [%d]: %f"), i, LidarValue);
                // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, FString::Printf(TEXT("Received LIDAR Data [%d]: %f"), i, LidarValue));
            }
        }
    }
    else
    {
        // UE_LOG(LogTemp, Warning, TEXT("Received data is less than expected size."));
    }
}

FReceivedData ASendUDPMessage::OnSendAndReceiveMessage(int32 TimeStamp, int32 CommandNumber, float InputData, int32 Port, const FString& Address)
{
    // Send the message and receive the response
    SendMessageToTSS(TimeStamp, CommandNumber, InputData, Port, Address);

    // Return an empty FReceivedData struct for now
    FReceivedData ReceivedData;
    return ReceivedData;
}

