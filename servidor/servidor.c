#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

#define BUFSIZE 512 // tamanho de buffer para o pipi, Obs: não usar o default

DWORD WINAPI InstanceThread(LPVOID lpvParam);

int _tmain(VOID) {
	BOOL	fConnected = FALSE;
	DWORD	dwThreadId = 0;
	HANDLE  hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
	LPTSTR	lpszPipename = TEXT("\\\\.\\pipe\\piexemplo");

	while (1) {
		_tprintf(TEXT("\nServidor  - ciclo principal - pipe= %s\n"), lpszPipename);

		hPipe = CreateNamedPipe(lpszPipename, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, BUFSIZE, BUFSIZE, 0, NULL);
		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("CreateNamedPipe falhou, erro=%d\n"), GetLastError());
			return -1;
		}

		fConnected = ConnectNamedPipe(hPipe, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

		if (fConnected) {
			_tprintf(TEXT("Cliente ligado. Vou criar uma thread para ele\n"));

			hThread = CreateThread(NULL, 0, InstanceThread, (LPVOID)hPipe, 0, &dwThreadId);

			if (hThread == NULL) {
				_tprintf(TEXT("Erro na criação da thread. erro=%d\n"), GetLastError());
				return -1;
			}
			else
				CloseHandle(hThread);
		}
		else
			CloseHandle(hPipe);
	}
	return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam) {
	HANDLE hHeap = GetProcessHeap();
	TCHAR* pchRequest = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	TCHAR* pchReply = (TCHAR*)HeapAlloc(hHeap, 0, BUFSIZE * sizeof(TCHAR));
	DWORD cbBytesRead = 0, cbReplyBytes = 0, cbWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hPipe = NULL;

	if (lpvParam == NULL) {
		_tprintf(TEXT("\nErro - o handle enviado no param, é nulo\n"));
		if (pchReply != NULL)
			HeapFree(hHeap, 0, pchReply);
		if (pchRequest != NULL)
			HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	if (pchRequest == NULL) {
		_tprintf(TEXT("\nErro - não houve memória para o pedido\n"));
		if (pchReply != NULL)
			HeapFree(hHeap, 0, pchReply);
		return (DWORD)-1;
	}

	if (pchReply == NULL) {
		_tprintf(TEXT("\nErro - não houve memória para a resposta\n"));
		if (pchRequest != NULL)
			HeapFree(hHeap, 0, pchRequest);
		return (DWORD)-1;
	}

	_tprintf(TEXT("Thread do servidor - a receber mensagem\n"));

	hPipe = (HANDLE)lpvParam;

	while (1) {
		fSuccess = ReadFile(hPipe, pchRequest, BUFSIZE * sizeof(TCHAR), &cbBytesRead, NULL);
		if (!fSuccess || cbBytesRead == 0) {
			if (GetLastError() == ERROR_BROKEN_PIPE)
				_tprintf(TEXT("Cliente desligou. Erro=%d\n"), GetLastError());
			else
				_tprintf(TEXT("ReadFile falhou. Erro=%d.\n"), GetLastError());
			break;
		}

		_tprintf(TEXT("\nMensagem recebida: %d bytes: \"%s\"\n"), (lstrlen(pchRequest) + 1) * sizeof(TCHAR), pchRequest);

		//cbWritten = (lstrlen(pchRequest) + 1) * sizeof(TCHAR);
		//fSuccess = WriteFile(hPipe, pchRequest, cbReplyBytes, &cbWritten, NULL);
		fSuccess = WriteFile(hPipe, pchReply, cbReplyBytes, &cbWritten, NULL);
		if (!fSuccess || cbReplyBytes != cbWritten) {
			_tprintf(TEXT("WriteFile Falhou, Erro=%d\n"), GetLastError());
			break;
		}
	}

	FlushFileBuffers(hPipe);
	DisconnectNamedPipe(hPipe);
	CloseHandle(hPipe);

	HeapFree(hHeap, 0, pchRequest);
	HeapFree(hHeap, 0, pchReply);

	_tprintf(TEXT("Thread a terminar"));
	return 1;
}