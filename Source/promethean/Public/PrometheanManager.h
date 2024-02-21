// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Sockets.h"

void SubscribeToVRKeyEvents(UWorld* InWorld);
bool SendSingleTCPMessage(const int32 TargetPort, FString Message);

bool LaunchTCP();
void TCPSocketListener();  //can thread this eventually?
void TCPConnectionListener();  //can thread this eventually?
bool StartTCPReceiver(
	const FString& YourChosenSocketName,
	const FString& TheIP,
	const int32 ThePort
);
FSocket* CreateTCPListenerSocket(
	const FString& YourChosenSocketName,
	const FString& TheIP,
	const int32 ThePort,
	const int32 ReceiveBufferSize = 2 * 1024 * 1024
);
void TCPCloseConnection();
void TCPSend(FString ToSend);

FString StringFromBinaryArray(TArray<uint8> BinaryArray);
bool FormatIP4ToNumber(const FString& TheIP, uint8(&Out)[4]);  //Format String IP4 to number array
