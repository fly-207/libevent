#ifndef _BASE_SERVER_HEAD_FILE_
#define _BASE_SERVER_HEAD_FILE_
#pragma once


class CBaseServer
{
public:
    CBaseServer();
    ~CBaseServer();

public:
    //
    void Init();
    //
    void Start();

private:
    //
    void InitNet();
    //
    void StartNet();

};


#endif

