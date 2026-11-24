// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "OculusXRColocation.h"
#include "OculusXRColocationModule.h"
#include "OculusXRColocationTypes.h"
#include "OculusXRColocationUtil.h"
#include "OculusXRColocationFunctions.h"
#include "OculusXRAsyncRequestSystem.h"

namespace OculusXRColocation
{
	TSharedPtr<FDiscoverSessionsRequest> FColocation::DiscoverSessionsAsync(const FDiscoverSessionsRequest::FCompleteDelegate& OnComplete, const FOculusXRColocationSessionFoundDelegate& OnSessionFound)
	{
		auto request = OculusXR::FAsyncRequestSystem::CreateRequest<FDiscoverSessionsRequest>();
		request->BindOnComplete(OnComplete);
		request->BindOnSessionFound(OnSessionFound);
		request->Execute();

		return request;
	}

	EColocationResult FColocation::StopDiscoverSessions(TSharedPtr<FDiscoverSessionsRequest> Request)
	{
		uint64 requestId = Request->GetRequestId().Id;
		EColocationResult colocationResult = IOculusXRColocationFunctions::GetOculusXRColocationFunctionsImpl()->StopColocationDiscovery(requestId);
		UE_LOG(LogOculusXRColocation, Log, TEXT("Stopping colocation session discovery. RequestID: %llu, Launch async result: %s"), Request->GetRequestId().Id, *ToString(colocationResult));

		if (colocationResult != EColocationResult::Success)
		{
			UE_LOG(LogOculusXRColocation, Warning, TEXT("Failed to stop local group discovery."));
		}

		return colocationResult;
	}

	TSharedPtr<FStartSessionAdvertisementRequest> FColocation::StartSessionAdvertisementAsync(const TArray<uint8>& SessionData, const FStartSessionAdvertisementRequest::FCompleteDelegate& OnComplete)
	{
		auto request = OculusXR::FAsyncRequestSystem::CreateRequest<FStartSessionAdvertisementRequest>(SessionData);
		request->BindOnComplete(OnComplete);
		request->Execute();
		return request;
	}

	TSharedPtr<FStopSessionAdvertisementRequest> FColocation::StopSessionAdvertisementAsync(const FStopSessionAdvertisementRequest::FCompleteDelegate& OnComplete)
	{
		auto request = OculusXR::FAsyncRequestSystem::CreateRequest<FStopSessionAdvertisementRequest>();
		request->BindOnComplete(OnComplete);
		request->Execute();
		return request;
	}
} // namespace OculusXRColocation
