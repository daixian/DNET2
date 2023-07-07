#pragma once

enum class DNetError
{
    Unknown = -1,
    Ok = 0,
    NotImplemented = 1,
    NotInitialized = 2,
    AlreadyInitialized = 3,
    InvalidParameter = 4,
    InvalidContext = 5,
    InvalidHandle = 6,
    RuntimeIncompatible = 7,
    RuntimeNotFound = 8,
    SymbolNotFound = 9,
    BufferTooSmall = 10,
    SyncFailed = 11,
    OperationFailed = 12,
    InvalidAttribute = 13,
};