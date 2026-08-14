// lagging packets
#include "iup.h"
#include "common.h"
#include "timing.h"
#define NAME "lag"
#define LAG_MIN "0"
#define LAG_MAX "3000"
#define KEEP_AT_MOST 2000
// send FLUSH_WHEN_FULL packets when buffer is full
#define FLUSH_WHEN_FULL 800
#define LAG_DEFAULT 50

// don't need a chance
static Ihandle *inboundCheckbox, *outboundCheckbox, *timeInput;

static volatile short lagEnabled = 0,
    lagInbound = 1,
    lagOutbound = 1;
// lagTime is stored in hundredths of a millisecond (FIXED_EPSILON = 0.01) so the
// UI can accept fractional ms like 2.5; e.g. 50ms is stored as 5000
static volatile LONG lagTime = LAG_DEFAULT * 100;

static PacketNode lagHeadNode = {0}, lagTailNode = {0};
static PacketNode *bufHead = &lagHeadNode, *bufTail = &lagTailNode;
static int bufSize = 0;

// QueryPerformanceCounter ticks/sec; queried once at startup. timeGetTime()'s
// whole-millisecond resolution isn't precise enough for the release-timing check
static LARGE_INTEGER lagQpcFrequency;

static INLINE_FUNCTION short isBufEmpty() {
    short ret = bufHead->next == bufTail;
    if (ret) assert(bufSize == 0);
    return ret;
}

static Ihandle *lagSetupUI() {
    Ihandle *lagControlsBox = IupHbox(
        inboundCheckbox = IupToggle("Inbound", NULL),
        outboundCheckbox = IupToggle("Outbound", NULL),
        IupLabel("Delay(ms):"),
        timeInput = IupText(NULL),
        NULL
        );

    IupSetAttribute(timeInput, "VISIBLECOLUMNS", "4");
    IupSetAttribute(timeInput, "VALUE", STR(LAG_DEFAULT));
    // uiSyncFixedInt accepts fractional ms (e.g. 2.5) and stores hundredths of a ms
    IupSetCallback(timeInput, "VALUECHANGED_CB", uiSyncFixedInt);
    IupSetAttribute(timeInput, SYNCED_VALUE, (char*)&lagTime);
    IupSetAttribute(timeInput, FIXED_MAX, LAG_MAX);
    IupSetAttribute(timeInput, FIXED_MIN, LAG_MIN);
    IupSetCallback(inboundCheckbox, "ACTION", (Icallback)uiSyncToggle);
    IupSetAttribute(inboundCheckbox, SYNCED_VALUE, (char*)&lagInbound);
    IupSetCallback(outboundCheckbox, "ACTION", (Icallback)uiSyncToggle);
    IupSetAttribute(outboundCheckbox, SYNCED_VALUE, (char*)&lagOutbound);

    // enable by default to avoid confusing
    IupSetAttribute(inboundCheckbox, "VALUE", "ON");
    IupSetAttribute(outboundCheckbox, "VALUE", "ON");

    if (parameterized) {
        setFromParameter(inboundCheckbox, "VALUE", NAME"-inbound");
        setFromParameter(outboundCheckbox, "VALUE", NAME"-outbound");
        setFromParameter(timeInput, "VALUE", NAME"-time");
    }

    return lagControlsBox;
}

static void lagStartUp() {
    if (bufHead->next == NULL && bufTail->next == NULL) {
        bufHead->next = bufTail;
        bufTail->prev = bufHead;
        bufSize = 0;
    } else {
        assert(isBufEmpty());
    }
    QueryPerformanceFrequency(&lagQpcFrequency);
    startTimePeriod();
}

static void lagCloseDown(PacketNode *head, PacketNode *tail) {
    PacketNode *oldLast = tail->prev;
    UNREFERENCED_PARAMETER(head);
    // flush all buffered packets
    LOG("Closing down lag, flushing %d packets", bufSize);
    while(!isBufEmpty()) {
        insertAfter(popNode(bufTail->prev), oldLast);
        --bufSize;
    }
    endTimePeriod();
}

static short lagProcess(PacketNode *head, PacketNode *tail) {
    LARGE_INTEGER currentTime;
    LONGLONG lagTimeTicks;
    PacketNode *pac = tail->prev;
    QueryPerformanceCounter(&currentTime);
    // lagTime is in hundredths of a ms, so unitsPerSecond = 100 * 1000
    lagTimeTicks = lagValueToQpcTicks(lagTime, lagQpcFrequency.QuadPart, 100000);
    // pick up all packets and fill in the current time
    while (bufSize < KEEP_AT_MOST && pac != head) {
        if (checkDirection(pac->addr.Outbound, lagInbound, lagOutbound)) {
            LARGE_INTEGER now;
            QueryPerformanceCounter(&now);
            insertAfter(popNode(pac), bufHead)->timestamp = now.QuadPart;
            ++bufSize;
            pac = tail->prev;
        } else {
            pac = pac->prev;
        }
    }

    // try sending overdue packets from buffer tail
    while (!isBufEmpty()) {
        pac = bufTail->prev;
        if (lagIsDue(currentTime.QuadPart, pac->timestamp, lagTimeTicks)) {
            insertAfter(popNode(bufTail->prev), head); // sending queue is already empty by now
            --bufSize;
            LOG("Send lagged packets.");
        } else {
            LOG("Sent some lagged packets, still have %d in buf", bufSize);
            break;
        }
    }

    // if buffer is full just flush things out
    if (bufSize >= KEEP_AT_MOST) {
        int flushCnt = FLUSH_WHEN_FULL;
        while (flushCnt-- > 0) {
            insertAfter(popNode(bufTail->prev), head);
            --bufSize;
        }
    }

    return bufSize > 0;
}

Module lagModule = {
    "Lag",
    NAME,
    (short*)&lagEnabled,
    lagSetupUI,
    lagStartUp,
    lagCloseDown,
    lagProcess,
    // runtime fields
    0, 0, NULL
};