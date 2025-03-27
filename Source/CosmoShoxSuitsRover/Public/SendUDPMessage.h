#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Common/UdpSocketReceiver.h" // For FArrayReaderPtr
#include "Common/UdpSocketBuilder.h" // For FIPv4Endpoint
#include "SendUDPMessage.generated.h"

USTRUCT(BlueprintType)
struct FReceivedData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 TimeStamp;

    UPROPERTY(BlueprintReadOnly)
    int32 CommandNumber;

    UPROPERTY(BlueprintReadOnly)
    float FloatData;

    UPROPERTY(BlueprintReadOnly)
    int32 IntData;

    UPROPERTY(BlueprintReadOnly)
    bool bIsFloat;

    UPROPERTY(BlueprintReadOnly)
    TArray<float> LidarData;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDataReceivedEvent, float, ReceivedFloatData);

UCLASS()
class COSMOSHOXSUITSROVER_API ASendUDPMessage : public AActor
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category = "Events")
    FOnDataReceivedEvent OnDataReceivedEvent;

    UFUNCTION(BlueprintCallable, Category = "UDP")
    void SendMessageToTSS(int32 TimeStamp, int32 CommandNumber, float InputData, int32 Port, const FString& Address);

    UFUNCTION(BlueprintCallable, Category = "UDP")
    FReceivedData OnSendAndReceiveMessage(int32 TimeStamp, int32 CommandNumber, float InputData, int32 Port, const FString& Address);

private:
    void StartReceiving(FSocket* Socket);
    void OnDataReceived(const FArrayReaderPtr& Data, const FIPv4Endpoint& Endpoint);

    TSharedPtr<FUdpSocketReceiver> UDPReceiver;
};

