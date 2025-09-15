// ********************************************************
// ** raiddrinit.c **
//
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// Version: 1.0.4
// Author: Brett Dodds (brett.dodds@microsoft.com)
// 
// For easy RTL creation of different Basic RAIDDR
// implementations.
// ********************************************************
#if defined(_WIN32)
    #define SCANF           scanf_s
    #define SCANFS(a,b,c)   scanf_s(a,b,c)
    #define STRNCPY(a,b,c)  strncpy_s(a,c,b,c)
#elif defined(__linux__)
    #define SCANF           scanf
    #define SCANFS(a,b,c)   scanf(a,b)
    #define STRNCPY         strncpy
#endif

#define MAX_PATH        512

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

typedef struct _RAIDDR_INPUTS
{
    int32_t cwBits;
    int32_t dataBits;
    int32_t symbolBits;
    int32_t subCodewords;
} RAIDDR_INPUTS, *PRAIDDR_INPUTS;

#define VIERROR_SUCCESS             0
#define VIERROR_DATA_GT_CW          -1
#define VIERROR_DATAPLUSSYM_GT_CW   -2
#define VIERROR_SYM_GT_HALFCW       -3
#define VIERROR_SYM_GT_DATA         -4
#define VIERROR_SYM_MULT_CW         -5
#define VIERROR_SCW_MULT_SYM        -6
int32_t ValidateInputs(RAIDDR_INPUTS ri)
{
    if (ri.cwBits > 0 && ri.dataBits > 0)
    {
        if (ri.dataBits >= ri.cwBits)
            return VIERROR_DATA_GT_CW;
        if (ri.symbolBits > 0 && ri.cwBits <= ri.dataBits + ri.symbolBits)
            return VIERROR_DATAPLUSSYM_GT_CW;
    }
    if (ri.cwBits > 0 && ri.symbolBits > 0 && ri.symbolBits > (ri.cwBits >> 1))
        return VIERROR_SYM_GT_HALFCW;
    if (ri.dataBits > 0 && ri.symbolBits > 0 && ri.symbolBits > ri.dataBits)
        return VIERROR_SYM_GT_DATA;
    if (ri.cwBits > 0 && ri.symbolBits > 0 && (ri.cwBits % ri.symbolBits) != 0)
        return VIERROR_SYM_MULT_CW;
    if (ri.subCodewords > 0 && (ri.symbolBits % ri.subCodewords) != 0)
        return VIERROR_SCW_MULT_SYM;
    return VIERROR_SUCCESS;
}

typedef struct _RAIDDR_ORG
{
    int32_t codewords;
    int32_t symbols;
    int32_t symbolSize;
    int32_t lastCwCrcSize;
} RAIDDR_ORG, *PRAIDDR_ORG;

void showhelp()
{
    printf("**** raiddrinit ****\n");
    printf(" RAIDDR Initialization Utility\n");
    printf("Usage: raiddrinit [options]\n");
    printf("Options:\n");
    printf("  -c <value>  Number of total bits in the codeword\n");
    printf("  -d <value>  Number of total data bits (data + metadata)\n");
    printf("  -s <value>  Number of bits that make up a symbol\n");
    printf("  -p <value>  Number of sub codewords (0 for optimized)\n");
    printf("  -o <file>   Output file (default: stdout)\n");
    printf("  -?          Show this help\n");
}

#define SIMPLE_POLYNOMIALS  0 // Comment out or set to 0 to use more complex polynomials
#if defined(SIMPLE_POLYNOMIALS) && SIMPLE_POLYNOMIALS == 1 // Simple polynomials for CRC generation
// Note, these polynomials may not be the most optimal.
// We suggest you review ECC gaps and determine whether a polynomial with more terms would be better.
static const uint64_t __primitiveRevPoly[65] = {
    0x0000000000000000,    //0x1,
    0x0000000000000001,    //0x3,
    0x0000000000000003,    //0x7,
    0x0000000000000006,    //0xB,
    0x000000000000000C,    //0x13,
    0x0000000000000014,    //0x25,
    0x0000000000000030,    //0x43,
    0x0000000000000060,    //0x83,
    0x00000000000000E1,    //0x187,
    0x0000000000000110,    //0x211,
    0x0000000000000240,    //0x409,
    0x0000000000000500,    //0x805,
    0x0000000000000E02,    //0x1407,
    0x0000000000001290,    //0x2129,
    0x0000000000003006,    //0x5803,
    0x0000000000006000,    //0x8003,
    0x0000000000008029,    //0x19401,
    0x0000000000012000,    //0x20009,
    0x0000000000020400,    //0x40081,
    0x0000000000048300,    //0x80609,
    0x0000000000090000,    //0x100009,
    0x0000000000140000,    //0x200005,
    0x0000000000300000,    //0x400003,
    0x0000000000420000,    //0x800021,
    0x0000000000A41000,    //0x1000825,
    0x0000000001200000,    //0x2000009,
    0x0000000002001404,    //0x480A001,
    0x0000000004000218,    //0x8C20001,
    0x0000000009000000,    //0x10000009,
    0x0000000014000000,    //0x20000005,
    0x0000000020180004,    //0x48000601,
    0x0000000048000000,    //0x80000009,
    0x00000000A1008000,    //0x100010085,
    0x0000000100080000,    //0x200002001,
    0x0000000202210000,    //0x400021101,
    0x0000000500000000,    //0x800000005,
    0x0000000801000000,    //0x1000000801,
    0x0000001400404000,    //0x2000404005,
    0x0000002180000400,    //0x4008000061,
    0x0000004400000000,    //0x8000000011,
    0x0000008000011400,    //0x10028800001,
    0x0000012000000000,    //0x20000000009,
    0x0000020000000C80,    //0x404C0000001,
    0x0000042000108000,    //0x80008400021,
    0x0000080002000110,    //0x108800040001,
    0x0000110000010020,    //0x208010000011,
    0x0000200008004020,    //0x410080040001,
    0x0000420000000000,    //0x800000000021,
    0x0000C04010000000,    //0x1000000080203,
    0x0001008000000000,    //0x2000000000201,
    0x0002000100048000,    //0x4000480020001,
    0x0004000804000010,    //0x8400001008001,
    0x0009000000000000,    //0x10000000000009,
    0x0010000100000804,    //0x24020000100001,
    0x0020000001000011,    //0x62000020000001,
    0x0040000040000000,    //0x80000001000001,
    0x0080000004014000,    //0x100028020000001,
    0x0102000000000000,    //0x200000000000081,
    0x0200004000000000,    //0x400000000080001,
    0x0400000100001010,    //0x840400004000001,
    0x0C00000000000000,    //0x1000000000000003,
    0x1000220000010000,    //0x2000100000088001,
    0x2400000800000010,    //0x4200000004000009,
    0x6000000000000000,    //0x8000000000000003,
    0x8040000020000004     //0x12000000400000201,
};
#else
// Polynomials for orders up to 31 & 64 selected from: https://users.ece.cmu.edu/~koopman/crc/notes.html (by Philip Koopman. Licensed under: https://creativecommons.org/licenses/by/4.0/)
// Polynomials for orders from 32 to 50 are brute-force generated irreducible polynomials 
// Polynomials for orders 51 to 63 selected from: https://poincare.matf.bg.ac.rs/~ezivkovm/publications/primpol1.pdf (Miodrag Zivkovic)
static const uint64_t __primitiveRevPoly[65] = {
    0x0000000000000000,    //0x1, // CRC-0 (Not useful, placeholder for indexing)
    // Selected from: https://users.ece.cmu.edu/~koopman/crc/notes.html (by Philip Koopman. Licensed under: https://creativecommons.org/licenses/by/4.0/)
    0x0000000000000001,    //0x3, // CRC-1
    0x0000000000000003,    //0x7, // CRC-2
    0x0000000000000006,    //0xB, // CRC-3
    0x000000000000000C,    //0x13, // CRC-4
    0x0000000000000017,    //0x3D, // CRC-5
    0x0000000000000039,    //0x67, // CRC-6
    0x0000000000000069,    //0xCB, // CRC-7
    0x00000000000000B2,    //0x14D, // CRC-8
    0x0000000000000198,    //0x233, // CRC-9
    0x00000000000003C9,    //0x64F, // CRC-10
    0x000000000000076E,    //0xBB7, // CRC-11
    0x0000000000000F0C,    //0x130F, // CRC-12
    0x0000000000001FD5,    //0x357F, // CRC-13
    0x0000000000003E7C,    //0x4F9F, // CRC-14
    0x000000000000713C,    //0x9E47, // CRC-15
    0x000000000000D4D8,    //0x11B2B, // CRC-16
    0x000000000001E5F6,    //0x2DF4F, // CRC-17
    0x000000000002D6FA,    //0x57DAD, // CRC-18
    0x0000000000057640,    //0x81375, // CRC-19
    0x00000000000FB57A,    //0x15EADF, // CRC-20
    0x00000000001CEBE3,    //0x38FAE7, // CRC-21
    0x00000000002D187B,    //0x77862D, // CRC-22
    0x0000000000773C90,    //0x849E77, // CRC-23
    0x0000000000F969E4,    //0x127969F, // CRC-24
    0x000000000131C9EB,    //0x3AF2719, // CRC-25
    0x0000000003F3B836,    //0x5B0773F, // CRC-26
    0x0000000007A45C71,    //0xC71D12F, // CRC-27
    0x000000000BE5FAD4,    //0x12B5FA7D, // CRC-28
    0x0000000014602AF8,    //0x23EA80C5, // CRC-29
    0x0000000037313B4A,    //0x54B7233B, // CRC-30
    0x0000000065D70BF4,    //0x97E875D3, // CRC-31
    // Brute-force generated irreducible polynomials
    0x00000000FFF3C0AB,    //0x1D503CFFF, // CRC-32
    0x000000017874955F,    //0x3F5525C3D, // CRC-33
    0x0000000249E7A288,    //0x445179E49, // CRC-34
    0x0000000676E2FA2D,    //0xDA2FA3B73, // CRC-35
    0x0000000903E73D7E,    //0x17EBCE7C09, // CRC-36
    0x0000001B68237399,    //0x3339D882DB, // CRC-37
    0x0000003AA52B13D4,    //0x4AF2352957, // CRC-38
    0x00000060E9DCA83A,    //0xAE0A9DCB83, // CRC-39
    0x00000086D9ED1289,    //0x19148B79B61, // CRC-40
    0x0000015A45748E1A,    //0x2B0E25D44B5, // CRC-41
    0x0000036FB8B55873,    //0x7386AB477DB, // CRC-42
    0x0000048A8264F555,    //0xD5579320A89, // CRC-43
    0x00000EF573B3D6DA,    //0x15B6BCDCEAF7, // CRC-44
    0x0000165A8B40D7A1,    //0x30BD605A2B4D, // CRC-45
    0x000032E36705DDD4,    //0x4AEEE839B1D3, // CRC-46
    0x00004AD642B83C1F,    //0xFC1E0EA135A9, // CRC-47
    0x0000C20732AA582C,    //0x1341A554CE043, // CRC-48
    0x0001301ABB7FAFA5,    //0x34BEBFDBAB019, // CRC-49
    0x0003DDC22926D9E6,    //0x59E6D92510EEF, // CRC-50
    // Selected from: https://poincare.matf.bg.ac.rs/~ezivkovm/publications/primpol1.pdf (Miodrag Zivkovic)
    0x0004004816000000,    //0x8000003409001, // CRC-51
    0x000E000804000002,    //0x14000002010007, // CRC-52
    0x0011000400808002,    //0x28002020040011, // CRC-53
    0x0020180060080000,    //0x40000401800601, // CRC-54
    0x0040004080000608,    //0x88300000810001, // CRC-55
    0x0084000808020400,    //0x100204010100021, // CRC-56
    0x0118000002010040,    //0x204010080000031, // CRC-57
    0x020000040210000C,    //0x4C0002100800001, // CRC-58
    0x0400003001002020,    //0x820200400600001, // CRC-59
    0x0800C10010000800,    //0x1001000080083001, // CRC-60
    0x1000000008402102,    //0x2810804200000001, // CRC-61
    0x2810280000002000,    //0x4001000000050205, // CRC-62
    0x4240110000000004,    //0x9000000000440121, // CRC-63
    // Selected from: https://users.ece.cmu.edu/~koopman/crc/notes.html (by Philip Koopman. Licensed under: https://creativecommons.org/licenses/by/4.0/)
    0x95AC9329AC4BC9B5     //0x1AD93D23594C935A9 // CRC-64
};
#endif

int32_t GenerateAlphaTable(int32_t bits, int32_t crcBits, uint64_t* p)
{
    uint64_t poly = __primitiveRevPoly[crcBits];
    uint64_t v = p[--bits] = poly;
    while (bits--)
        v = p[bits] = (v >> 1) ^ ((v & 1) ? poly : 0);
    return 0;
}

int main(int argc, char* argv[])
{
    int32_t arg, i, j, k, m, n, x, y, c;
    int32_t minCrcSize;
    int32_t crcSize;
    int32_t cwSize;
    int32_t metadataBits;
    char yn;
    RAIDDR_INPUTS ri;
    RAIDDR_ORG ro;
    uint64_t basepoly = 0xABCDEF0123456789;
    uint64_t tmp1, tmp2;
    uint64_t* pAlpha;
    uint64_t* pAlphaS;
    FILE* poutFile;
    char outFileName[MAX_PATH] = "";

    ri.cwBits = -1;
    ri.dataBits = -1;
    ri.symbolBits = -1;
    ri.subCodewords = -1;

    // Process command-line variables
    for (arg = 1; arg < argc; arg++)
    {
        if (argv[arg][0] == '-' || argv[arg][0] == '/')
        {
            // For switches that don't have a value after them...
            if (argv[arg][1] == '?' || argv[arg][1] == 'h' || (argv[arg][1] == '-' && argv[arg][2] == 'h')) // This is the help switch first
            {
                showhelp();
                return 0;
            }
            // else if () // For other switches that are boolean
            else
            {
                // Make sure there is data behind this
                #if defined(_WIN32)
                if (arg + 1 >= argc || argv[arg + 1][0] == '-' || argv[arg + 1][0] == '/')
                #else
                if (arg + 1 >= argc || argv[arg + 1][0] == '-')
                #endif
                {
                    printf("Invalid parameter specified after option '%c'\n", argv[arg][1]);
                    showhelp();
                    return 1;
                }
                switch (argv[arg][1])
                {
                    case 'c':
                        ri.cwBits = atoi(argv[arg + 1]);
                        break;
                    case 'd':
                        ri.dataBits = atoi(argv[arg + 1]);
                        break;
                    case 's':
                        ri.symbolBits = atoi(argv[arg + 1]);
                        break;
                    case 'p':
                        ri.subCodewords = atoi(argv[arg + 1]);
                        break;
                    case 'o':
                        STRNCPY(outFileName, argv[arg + 1], MAX_PATH);
                        break;
                    case '?':
                        showhelp();
                        return 0;
                    default:
                        printf("Unknown option: %s\n", argv[arg]);
                        showhelp();
                        return 1;
                }
                arg++;
            }
        }
    }

    // Gather input for non-specified variables
    while (ri.cwBits < 0)
    {
        printf("Enter the number of total bits in the codeword: ");
        SCANF("%d", &ri.cwBits);
    }
    while (ri.dataBits < 0)
    {
        printf("Enter the total number of data bits (data + metadata): ");
        SCANF("%d", &ri.dataBits);
    }
    while (ri.symbolBits < 0)
    {
        printf("Enter the number of bits that make up a symbol: ");
        SCANF("%d", &ri.symbolBits);
    }
    while (ri.subCodewords < 0)
    {
        printf("Enter the number of sub-codewords (or 0 for optimized): ");
        SCANF("%d", &ri.subCodewords);
    }

    // Validate inputs
    i = ValidateInputs(ri);
    if (i == VIERROR_DATAPLUSSYM_GT_CW)
    {
        printf("Problem: Specified symbol size is too large. Adjust symbol size to maximize capability? (y/n): ");
        if (SCANFS(" %c", &yn, 1) != 1)
        {
            printf("Response error");
            return 1;
        }
        if (yn == 'y' || yn == 'Y')
        {
            ri.symbolBits = ((ri.cwBits - ri.dataBits) + 1) / 2;
            i = ValidateInputs(ri);
        }
        else
            return 1;
    }
    if (i < 0)
    {
        switch (i)
        {
            case VIERROR_DATA_GT_CW:
                printf("Invalid input: Number of data bits must be less than the number of bits in the codeword\n");
                break;
            case VIERROR_DATAPLUSSYM_GT_CW:
                printf("Invalid input: Number of bits in the codeword must be greater than the number of data bits plus the number of bits that make up a symbol\n");
                break;
            case VIERROR_SYM_GT_HALFCW:
                printf("Invalid input: Number of bits that make up a symbol must be less than half the number of bits in the codeword\n");
                break;
            case VIERROR_SYM_GT_DATA:
                printf("Invalid input: Number of bits that make up a symbol must be less than the number of data bits\n");
                break;
            case VIERROR_SYM_MULT_CW:
                printf("Invalid input: Number of bits in the codeword must be a multiple of the number of bits that make up a symbol\n");
                break;
            case VIERROR_SCW_MULT_SYM:
                printf("Invalid input: Number of sub codewords must evenly split the symbol size\n");
                break;
        }
        return 1;
    }

    // Derive organization
    ro.symbols = ri.cwBits / ri.symbolBits;
    minCrcSize = 1;
    while (1 << minCrcSize < ro.symbols)
        minCrcSize++;
    crcSize = ri.cwBits - ri.dataBits - ri.symbolBits; // CRC Size tells us how many bits we have to be able to do CRC
    if (crcSize < minCrcSize)
    {
        printf("Error with input parameters: Note enough bits for ECC\n");
        return 1;
    }
    // Find the right CW size
    if (ri.subCodewords > 0)
    {
        ro.codewords = ri.subCodewords;
        ro.symbolSize = ri.symbolBits / ro.codewords;
    }
    else
    {
        ro.symbolSize = ri.symbolBits - crcSize + 1;
        if (ro.symbolSize < minCrcSize)
            ro.symbolSize = minCrcSize;
        while (ri.symbolBits % ro.symbolSize)
            ro.symbolSize++;
        ro.codewords = ri.symbolBits / ro.symbolSize;
    }
    ro.lastCwCrcSize = ro.symbolSize - (ro.codewords * ro.symbolSize - crcSize);

    // Validate the lastCwCrcSize is big enough to distinguish a symbol, if not... need to decrease the number of subcodewords
    while ((1 << ro.lastCwCrcSize) < (ro.symbols * (ro.symbolSize - 1)))
    {
        do
        {
            ro.symbolSize++;
        } while (ri.symbolBits % ro.symbolSize);
        ro.codewords = ri.symbolBits / ro.symbolSize;
        ro.lastCwCrcSize = ro.symbolSize - (ro.codewords * ro.symbolSize - crcSize);
    }

    // Show summary
    printf("Summary:\n");
    printf("  CodeWord:    %d bits\n", ri.cwBits);
    printf("  Data:        %d bits\n", ri.dataBits);
    printf("  ECC:         %d bits\n", ri.cwBits - ri.dataBits);
    printf("  Symbol Size: %d bits\n", ri.symbolBits);
    printf("  Symbols:     %d\n", ro.symbols);
    printf("  Sub-CodeWords:   %d\n", ro.codewords);
    printf("  Sub-SymbolSize:  %d bits\n", ro.symbolSize);
    printf("  LastCwCrcSize:   %d bits\n", ro.lastCwCrcSize);

    // Print visual representation of the codeword
    x = 1;
    while (x * x <= ro.symbolSize)
        x++;
    x--;
    while (x > 1)
    {
        if ((ro.symbolSize % x) == 0)
            break;
        else
            x--;
    }
    y = ro.symbolSize / x;
    if (x < y) // Favor width over height
    {
        y = x;
        x = ro.symbolSize / y;
    }

    printf("Codeword Layout:\n");
    printf("    Symbol\nCW  ");
    for (j = 0; j < ro.symbols; j++)
        printf("%*d", x + 1, j);
    for (m = 0; m < ro.codewords; m++)
    {
        for (i = 0; i < y; i++)
        {
            printf("\n");
            if (i == 0)
                printf("%2d  ", m);
            else
                printf("    ");
            for (j = 0; j < ro.symbols; j++)
            {
                printf(" ");
                for (k = 0; k < x; k++)
                {
                    n = j * ro.symbolSize * ro.codewords + m * ro.symbolSize + i * x + k;
                    if (j == ro.symbols - 1)
                        c = 'X';
                    else if (n >= ro.symbolSize * ro.codewords * (ro.symbols - 2) + ro.symbolSize - ro.lastCwCrcSize)
                    //else if (j == ro.symbols - 2 && ((m * y + i) * x + k) < ro.symbolSize - ro.lastCwCrcSize)
                        c = (m & 1) ? 'R' : 'r';
                    else
                        c = j + ((m & 1) ? 'A' : 'a');
                    printf("%c", c);
                }
            }
        }
    }

    // Generate alpha values - Do this for upper CRC's and the lower CRC
    printf("\nGenerating CRC alpha values...\n");
    cwSize = (ro.symbols - 1) * ro.symbolSize;
    if (ro.codewords > 1)
    {
        pAlpha = malloc(sizeof(uint64_t) * cwSize);
        pAlphaS = malloc(sizeof(uint64_t) * cwSize);
        GenerateAlphaTable(cwSize, ro.symbolSize, pAlpha);
        GenerateAlphaTable(cwSize, ro.lastCwCrcSize, pAlphaS);
    }
    else
    {
        pAlpha = malloc(sizeof(uint64_t) * cwSize);
        GenerateAlphaTable(cwSize, ro.lastCwCrcSize, pAlpha);
    }

    // Generate the Verilog code
    printf("\nGenerating RTL code...\n");
    metadataBits = ro.symbolSize - ro.lastCwCrcSize;
    if (outFileName[0] == 0)
        poutFile = stdout;
    else
    {
        #if defined(_WIN32)
            fopen_s(&poutFile, outFileName, "w");
        #else
            poutFile = fopen(outFileName, "w");
        #endif
        if (poutFile == NULL)
        {
            printf("Error opening output file: %s\n", outFileName);
            printf("...Using stdout instead\n");
            poutFile = stdout;
        }
        printf("Saving output to: %s\n", outFileName);
    }
    if (ro.codewords > 1)
    {
        fprintf(poutFile,
            "// Basic RAIDDR RTL File generated by \"raiddrinit\" (C) Microsoft Corp.\n"
            "// Questions? Contact: Brett Dodds (brett.dodds@microsoft.com)\n"
            "// Settings:\n"
            "//   Symbols:          %d\n"
            "//   Symbol size:      %d bits\n"
            "//   Sub-CodeWords:    %d\n"
            "//   Metadata bits:    %d\n"
            "//   Symbol UE gaps:   %llu\n"
            , ro.symbols
            , ro.symbolSize * ro.codewords
            , ro.codewords
            , metadataBits
            , (1ULL << metadataBits) - 1
        );
    }
    else
    {
        fprintf(poutFile,
            "// Basic RAIDDR RTL File generated by \"raiddrinit\" (C) Microsoft Corp.\n"
            "// Questions? Contact: Brett Dodds (brett.dodds@microsoft.com)\n"
            "// Settings:\n"
            "//   Symbols:          %d\n"
            "//   Symbol size:      %d bits\n"
            "//   Metadata bits:    %d\n"
            "//   Symbol UE gaps:   %llu\n"
            , ro.symbols
            , ro.symbolSize
            , metadataBits
            , (1ULL << metadataBits) - 1
        );
    }
    if (metadataBits < 6) // Up to 31 gap patterns, print them out
    {
        tmp1 = (((ro.codewords > 1) ? pAlphaS[cwSize - 1] : pAlpha[cwSize - 1]) << 1) | 1;
        for (i = 1; i < (1 << metadataBits); i++)
        {
            tmp2 = 0;
            for (j = 0; j < metadataBits; j++)
            {
                if (i & (1 << j))
                    tmp2 ^= (tmp1 << j);
            }
            if (ro.codewords > 1)
            {
                fprintf(poutFile, "//     0x");
                k = ro.symbolSize * ro.codewords;
                while (k > 0)
                {
                    m = 0;
                    do
                    {
                        m <<= 1;
                        k--;
                        m |= (tmp2 >> (k % ro.symbolSize)) & 1;
                    } while (k & 3);
                    fprintf(poutFile, "%x", m);
                }
            }
            else
                fprintf(poutFile, "//     0x%.*" PRIx64, (ro.symbolSize + 3) >> 2, tmp2);
            fprintf(poutFile, "\n");
        }
    }

    if (ro.codewords >  1)
    {
        fprintf(poutFile,
            "\nmodule raiddr_crcconstants_%dx%dx%d_%dm(\n"
            "    output reg [%d:0] v[%d:0],\n"
            "    output reg [%d:0] vs[%d:0]\n"
            ");\n"
            "    assign v = {"
            , ro.symbols, ro.symbolSize, ro.codewords, metadataBits
            , ro.symbolSize - 1, cwSize - 1
            , ro.lastCwCrcSize - 1, cwSize - 1
        );
    }
    else
    {
        fprintf(poutFile,
            "\nmodule raiddr_crcconstants_%dx%d_%dm(\n"
            "    output reg [%d:0] v[%d:0]\n"
            ");\n"
            "    assign v = {"
            , ro.symbols, ro.symbolSize, metadataBits
            , ro.lastCwCrcSize - 1, cwSize - 1
        );
    }
    for (i = 0; i < cwSize; i++)
    {
        if (i)
            fprintf(poutFile, ",");
        if ((i & 7) == 0)
            fprintf(poutFile, "\n       ");
        fprintf(poutFile, " %d'h%.*" PRIx64, ((ro.codewords > 1) ? ro.symbolSize : ro.lastCwCrcSize), (ro.symbolSize + 3) >> 2, pAlpha[cwSize - 1 - i]);
    }
    if (ro.codewords > 1)
    {
        fprintf(poutFile, 
            "\n    };\n"
            "    assign vs = {"
        );
        for (i = 0; i < cwSize; i++)
        {
            if (i)
                fprintf(poutFile, ",");
            if ((i & 7) == 0)
                fprintf(poutFile, "\n       ");
            fprintf(poutFile, " %d'h%.*" PRIx64, ro.lastCwCrcSize, (ro.lastCwCrcSize + 3) >> 2, pAlphaS[cwSize - 1 - i]);
        }
    }
    fprintf(poutFile,
        "\n    };\n"
        "endmodule\n"
    );
    
    free(pAlpha);
    if (ro.codewords > 1)
        free(pAlphaS);
    
    if (ro.codewords > 1)
    {
        fprintf(poutFile,
            "\nmodule raiddr_enc(\n"
            "    input [%d:0] data,\n"
            "    input [%d:0] metadata,\n"
            "    output [%d:0] codeword\n"
            ");\n"
            "    wire [%d:0] crcconstants[%d:0];\n"
            "    wire [%d:0] crcconstants_s[%d:0];\n"
            "    raiddr_crcconstants_%dx%dx%d_%dm crcconstants_inst(\n"
            "       .v(crcconstants),\n"
            "       .vs(crcconstants_s)\n"
            "    );\n"
            "    raiddr_encode #(.S_SZ(%d), .CW(%d), .MD(%d), .S_COUNT(%d)) encoder(\n"
            "        .data(data),\n"
            "        .metadata(metadata),\n"
            "        .crcconstants(crcconstants),\n"
            "        .crcconstants_s(crcconstants_s),\n"
            "        .codeword(codeword)\n"
            "    );\n"
            "endmodule\n"
            , ri.dataBits - metadataBits - 1
            , metadataBits - 1
            , ro.codewords * ro.symbolSize * ro.symbols - 1
            , ro.symbolSize - 1, ro.symbolSize* (ro.symbols - 1) - 1
            , ro.lastCwCrcSize - 1, ro.symbolSize* (ro.symbols - 1) - 1
            , ro.symbols, ro.symbolSize, ro.codewords, metadataBits
            , ro.symbolSize, ro.codewords, metadataBits, ro.symbols
        );
        fprintf(poutFile,
            "\nmodule raiddr_dec(\n"
            "    input [%d:0] codeword,\n"
            "    output [%d:0] data,\n"
            "    output [%d:0] metadata,\n"
            "    output reg ue,\n"
            "    output reg ce,\n"
            "    output reg [%d:0] ceMask,\n"
            "    output reg [%d:0] symMask\n"
            ");\n"
            "    wire [%d:0] crcconstants[%d:0];\n"
            "    wire [%d:0] crcconstants_s[%d:0];\n"
            "    raiddr_crcconstants_%dx%dx%d_%dm crcconstants_inst(\n"
            "       .v(crcconstants),\n"
            "       .vs(crcconstants_s)\n"
            "    );\n"
            "    raiddr_decode #(.S_SZ(%d), .CW(%d), .MD(%d), .S_COUNT(%d)) decoder(\n"
            "        .codeword(codeword),\n"
            "        .crcconstants(crcconstants),\n"
            "        .crcconstants_s(crcconstants_s),\n"
            "        .data(data),\n"
            "        .metadata(metadata),\n"
            "        .ue(ue),\n"
            "        .ce(ce),\n"
            "        .ceMask(ceMask),\n"
            "        .symMask(symMask)\n"
            "    );\n"
            "endmodule\n"
            "\n"
            "// *** End of Generated RAIDDR Initialization\n"
            "// --- Append contents of 'basicraiddr_rtlbase_multicw.v' to this file ---\n"
            , ro.codewords * ro.symbolSize * ro.symbols - 1
            , ri.dataBits - metadataBits - 1
            , metadataBits - 1
            , ro.symbolSize * ro.codewords - 1
            , ro.symbols - 1
            , ro.symbolSize - 1, ro.symbolSize * (ro.symbols - 1) - 1
            , ro.lastCwCrcSize - 1, ro.symbolSize * (ro.symbols - 1) - 1
            , ro.symbols, ro.symbolSize, ro.codewords, metadataBits
            , ro.symbolSize, ro.codewords, metadataBits, ro.symbols
        );
    }
    else // Single code-word
    {
        fprintf(poutFile,
            "\nmodule raiddr_enc(\n"
            "    input [%d:0] data,\n"
            "    input [%d:0] metadata,\n"
            "    output [%d:0] codeword\n"
            ");\n"
            "    wire [%d:0] crcconstants[%d:0];\n"
            "    raiddr_crcconstants_%dx%d_%dm crcconstants_inst(\n"
            "       .v(crcconstants)\n"
            "    );\n"
            "    raiddr_encode #(.S_SZ(%d), .MD(%d), .S_COUNT(%d)) encoder(\n"
            "        .data(data),\n"
            "        .metadata(metadata),\n"
            "        .crcconstants(crcconstants),\n"
            "        .codeword(codeword)\n"
            "    );\n"
            "endmodule\n"
            , ri.dataBits - metadataBits - 1
            , metadataBits - 1
            , ro.symbolSize * ro.symbols - 1
            , ro.lastCwCrcSize - 1, ro.symbolSize* (ro.symbols - 1) - 1
            , ro.symbols, ro.symbolSize, metadataBits
            , ro.symbolSize, metadataBits, ro.symbols
        );
        fprintf(poutFile,
            "\nmodule raiddr_dec(\n"
            "    input [%d:0] codeword,\n"
            "    output [%d:0] data,\n"
            "    output [%d:0] metadata,\n"
            "    output reg ue,\n"
            "    output reg ce,\n"
            "    output reg [%d:0] ceMask,\n"
            "    output reg [%d:0] symMask\n"
            ");\n"
            "    wire [%d:0] crcconstants[%d:0];\n"
            "    raiddr_crcconstants_%dx%d_%dm crcconstants_inst(\n"
            "       .v(crcconstants)\n"
            "    );\n"
            "    raiddr_decode #(.S_SZ(%d), .MD(%d), .S_COUNT(%d)) decoder(\n"
            "        .codeword(codeword),\n"
            "        .crcconstants(crcconstants),\n"
            "        .data(data),\n"
            "        .metadata(metadata),\n"
            "        .ue(ue),\n"
            "        .ce(ce),\n"
            "        .ceMask(ceMask),\n"
            "        .symMask(symMask)\n"
            "    );\n"
            "endmodule\n"
            "\n"
            "// *** End of Generated RAIDDR Initialization\n"
            "// --- Append contents of 'basicraiddr_rtlbase_1cw.v' to this file ---\n"
            , ro.symbolSize * ro.symbols - 1
            , ri.dataBits - metadataBits - 1
            , metadataBits - 1
            , ro.symbolSize - 1
            , ro.symbols - 1
            , ro.lastCwCrcSize - 1, ro.symbolSize * (ro.symbols - 1) - 1
            , ro.symbols, ro.symbolSize, metadataBits
            , ro.symbolSize, metadataBits, ro.symbols
        );
    }

    return 0;
}

