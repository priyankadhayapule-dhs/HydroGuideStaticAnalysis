#include <errno.h>
#include <fcntl.h>
#include <linux/ion.h>
#include <ion/ion.h>

#ifdef DAU_EMULATED
    #define LOG_TAG "EmuDmaBuffer_test"
    #include <EmuDmaBuffer.h>
    #define DMATEST(test_case, test_name) TEST(Emu##test_case, test_name)
#else
    #define LOG_TAG "DauDmaBuffer_test"
    #include <PciDmaBuffer.h>
    #define DMATEST(test_case, test_name) TEST(Dau##test_case, test_name)
#endif

#include <utils/Log.h>

#include <gtest/gtest.h>

#define MAGIC_VALUE 0xDEADBEEF

namespace android {

DmaBuffer& getDmaBuffer()
{
#ifdef DAU_EMULATED
    return(EmuDmaBuffer::getInstance()); 
#else
    static PciDmaBuffer instance;

    return(instance); 
#endif
}

// Test successful opening of the DmaBuffer.  Failures can indicate
// issues with permissions, failure of the EIB driver, or missing/invalid
// Device Tree configuration.
DMATEST(ThorDmaBufferTest, AccessTest)
{
    DmaBuffer&  dmaBuffer = getDmaBuffer();

    EXPECT_TRUE(dmaBuffer.open());
    dmaBuffer.close();
}

// Verify successful access of DmaBuffer by comparing against known magic value
DMATEST(ThorDmaBufferTest, DataTest)
{
    DmaBuffer&  dmaBuffer = getDmaBuffer();
    uint32_t*   bufPtr;

    // Data should be invalid before opening
    EXPECT_EQ(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    EXPECT_TRUE(NULL == dmaBuffer.getBufferPtr(DmaBufferType::Fifo));
    EXPECT_EQ(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence));
    EXPECT_TRUE(NULL == dmaBuffer.getBufferPtr(DmaBufferType::Sequence));
    EXPECT_EQ(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Diagnostics));
    EXPECT_TRUE(NULL == dmaBuffer.getBufferPtr(DmaBufferType::Diagnostics));

    ASSERT_TRUE(dmaBuffer.open());

    // Data should be valid after opening
    EXPECT_NE(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    EXPECT_NE(0UL, dmaBuffer.getBufferLength(DmaBufferType::Fifo));
    bufPtr = dmaBuffer.getBufferPtr(DmaBufferType::Fifo);
    EXPECT_TRUE(NULL != bufPtr);
    EXPECT_EQ(MAGIC_VALUE, *bufPtr);

#ifndef DAU_EMULATED
    // These comparisons assume knowledge of internal layout of DmaBuffer to verify.
    EXPECT_NE(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence));
    EXPECT_NE(0UL, dmaBuffer.getBufferLength(DmaBufferType::Sequence));
    bufPtr = dmaBuffer.getBufferPtr(DmaBufferType::Sequence);
    EXPECT_TRUE(NULL != bufPtr);

    EXPECT_NE(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Diagnostics));
    EXPECT_NE(0UL, dmaBuffer.getBufferLength(DmaBufferType::Diagnostics));
    bufPtr = dmaBuffer.getBufferPtr(DmaBufferType::Diagnostics);
    EXPECT_TRUE(NULL != bufPtr);
#endif

    RecordProperty("Fifo Physical Address", dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    RecordProperty("Fifo Buffer Length", dmaBuffer.getBufferLength(DmaBufferType::Fifo));
    RecordProperty("Sequence Physical Address", dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence));
    RecordProperty("Sequence Buffer Length", dmaBuffer.getBufferLength(DmaBufferType::Sequence));
    RecordProperty("Diagnostics Physical Address", dmaBuffer.getPhysicalAddr(DmaBufferType::Diagnostics));
    RecordProperty("Diagnostics Buffer Length", dmaBuffer.getBufferLength(DmaBufferType::Diagnostics));

    dmaBuffer.close();
}

// Helper function to open needed files and put descriptors into struct
void FillInContext(DauContext* dauContextPtr)
{
    dauContextPtr->eibIoctlFd = ::open("/dev/eib_ioctl", O_RDWR | O_NONBLOCK);
    ASSERT_TRUE(-1 != dauContextPtr->eibIoctlFd);

    dauContextPtr->ionFd = ion_open();
    ASSERT_TRUE(-1 != dauContextPtr->ionFd);
}

#ifndef DAU_EMULATED
// Verify buffer access by using alternative form of open(DauContext)
DMATEST(ThorDmaBufferTest, DauContextTest)
{
    DmaBuffer&  dmaBuffer = getDmaBuffer();
    DauContext  dauContext;
    uint32_t*   bufPtr;

    // Data should be invalid
    EXPECT_EQ(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    EXPECT_TRUE(NULL == dmaBuffer.getBufferPtr(DmaBufferType::Fifo));
    EXPECT_EQ(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Sequence));
    EXPECT_TRUE(NULL == dmaBuffer.getBufferPtr(DmaBufferType::Sequence));
    EXPECT_EQ(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Diagnostics));
    EXPECT_TRUE(NULL == dmaBuffer.getBufferPtr(DmaBufferType::Diagnostics));

    // Open files and pass via DauContext
    FillInContext(&dauContext);

    ASSERT_TRUE(dmaBuffer.open(&dauContext));

    // Data should be valid after opening
    EXPECT_NE(0UL, dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    EXPECT_NE(0UL, dmaBuffer.getBufferLength(DmaBufferType::Fifo));
    bufPtr = dmaBuffer.getBufferPtr(DmaBufferType::Fifo);
    EXPECT_TRUE(NULL != bufPtr);
    EXPECT_EQ(MAGIC_VALUE, *bufPtr);

    RecordProperty("Physical Address", dmaBuffer.getPhysicalAddr(DmaBufferType::Fifo));
    RecordProperty("Buffer Length", dmaBuffer.getBufferLength(DmaBufferType::Fifo));

    dmaBuffer.close();
}
#endif

}


