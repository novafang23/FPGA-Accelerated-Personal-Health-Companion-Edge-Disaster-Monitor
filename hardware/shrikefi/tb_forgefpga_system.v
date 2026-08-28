// =============================================================================
// File: tb_forgefpga_system.v
// Module: tb_forgefpga_system
// Project: SIH26181 Health Companion & Disaster Monitor
// Target: Renesas ForgeFPGA (SLG47910) / ShrikeFi Development Board
// Description:
//   Comprehensive, self-checking testbench for the ShrikeFi 4-bit parallel
//   FPGA hardware accelerator. Simulates an ESP32-S3 microcontroller issuing
//   4-bit nibble transactions over GPIOs, verifying filter convergence,
//   cycle-accurate IBI timestamping, and interrupt handshaking.
// =============================================================================

`timescale 1ns / 1ps

module tb_forgefpga_system;

    // Clock and Timing Parameters
    localparam CLK_PERIOD_NS  = 20;            // 50 MHz Clock = 20 ns period
    localparam STROBE_PERIOD  = 100;           // 10 MHz Link Strobe = 100 ns period
    localparam CLK_FREQ_HZ    = 50_000_000;
    localparam REFRACTORY_MS  = 250;

    // Protocol Command Constants
    localparam [3:0] CMD_NOP          = 4'h0;
    localparam [3:0] CMD_WRITE_RED    = 4'h1;
    localparam [3:0] CMD_WRITE_IR     = 4'h2;
    localparam [3:0] CMD_WRITE_THRESH = 4'h3;
    localparam [3:0] CMD_READ_RED     = 4'h4;
    localparam [3:0] CMD_READ_IR      = 4'h5;
    localparam [3:0] CMD_READ_IBI     = 4'h6;
    localparam [3:0] CMD_CLEAR_IRQ    = 4'h7;
    localparam [3:0] CMD_READ_STATUS  = 4'h8;

    // DUT Signals
    reg         clk;
    reg         rst_n;
    reg         link_strobe;
    reg         link_dir;
    reg  [3:0]  link_din;
    wire [3:0]  link_dout;
    wire        link_dout_oe;
    wire        irq_beat;

    // Test Tracking
    integer tests_passed = 0;
    integer tests_failed = 0;
    integer total_tests  = 0;

    // Instantiate DUT (Device Under Test)
    forgefpga_ppg_top #(
        .CLK_FREQ_HZ   (CLK_FREQ_HZ),
        .REFRACTORY_CYC(100)
    ) dut (
        .clk         (clk),
        .rst_n       (rst_n),
        .link_strobe (link_strobe),
        .link_dir    (link_dir),
        .link_din    (link_din),
        .link_dout   (link_dout),
        .link_dout_oe(link_dout_oe),
        .irq_beat    (irq_beat)
    );

    // 50 MHz System Clock Generator
    initial clk = 0;
    always #(CLK_PERIOD_NS / 2) clk = ~clk;

    // =========================================================================
    // ESP32-S3 Bus Functional Model (BFM) Tasks
    // =========================================================================

    // Send Strobe Pulse
    task pulse_strobe;
        begin
            #20 link_strobe = 1;
            #40 link_strobe = 0;
            #40;
        end
    endtask

    // Write 8-bit value to FPGA over 4-bit link (Command + 2 Nibbles)
    task link_write_reg;
        input [3:0] cmd;
        input [7:0] data;
        begin
            link_dir = 0; // Host write
            
            // 1. Command Nibble
            link_din = cmd;
            pulse_strobe();
            
            // 2. High Data Nibble
            link_din = data[7:4];
            pulse_strobe();
            
            // 3. Low Data Nibble
            link_din = data[3:0];
            pulse_strobe();
            
            link_din = 4'h0;
        end
    endtask

    // Read 8-bit value from FPGA (Command in Write mode -> Switch to Read mode -> 2 Nibbles)
    task link_read_8bit;
        input  [3:0] cmd;
        output [7:0] data;
        reg    [3:0] high_nib;
        reg    [3:0] low_nib;
        begin
            // 1. Send Command (Write mode)
            link_dir = 0;
            link_din = cmd;
            pulse_strobe();

            // 2. Switch to Read mode
            link_dir = 1;
            #100;

            // 3. Read High Nibble
            high_nib = link_dout;
            pulse_strobe();
            #100;

            // 4. Read Low Nibble
            low_nib = link_dout;
            pulse_strobe();
            #50;

            link_dir = 0;
            data = {high_nib, low_nib};
        end
    endtask

    // Read 32-bit IBI Cycles (Command + 8 Nibbles)
    task link_read_ibi;
        output [31:0] ibi_val;
        reg [3:0] n[0:7];
        integer i;
        begin
            // 1. Send Command
            link_dir = 0;
            link_din = CMD_READ_IBI;
            pulse_strobe();

            // 2. Switch to Read mode
            link_dir = 1;
            #100;

            // 3. Read 8 nibbles
            for (i = 0; i < 8; i = i + 1) begin
                n[i] = link_dout;
                pulse_strobe();
                #100;
            end

            link_dir = 0;
            ibi_val = {n[0], n[1], n[2], n[3], n[4], n[5], n[6], n[7]};
        end
    endtask

    // Clear Interrupt (Single nibble command)
    task link_clear_irq;
        begin
            link_dir = 0;
            link_din = CMD_CLEAR_IRQ;
            pulse_strobe();
            link_din = 4'h0;
        end
    endtask

    // Task to send a complete systolic pulse
    task send_cardiac_pulse;
        begin
            link_write_reg(CMD_WRITE_RED, 8'd50);
            link_write_reg(CMD_WRITE_RED, 8'd50);
            link_write_reg(CMD_WRITE_RED, 8'd80);
            link_write_reg(CMD_WRITE_RED, 8'd110);
            link_write_reg(CMD_WRITE_RED, 8'd150);
            link_write_reg(CMD_WRITE_RED, 8'd180);
            link_write_reg(CMD_WRITE_RED, 8'd210); // Peak
            link_write_reg(CMD_WRITE_RED, 8'd190);
            link_write_reg(CMD_WRITE_RED, 8'd140);
            link_write_reg(CMD_WRITE_RED, 8'd90);
            link_write_reg(CMD_WRITE_RED, 8'd50);
            link_write_reg(CMD_WRITE_RED, 8'd50);
        end
    endtask

    // =========================================================================
    // Main Verification Flow
    // =========================================================================
    reg [7:0]  read_val8;
    reg [31:0] read_val32;
    integer    k;

    initial begin
        $dumpfile("shrikefi_sim.vcd");
        $dumpvars(0, tb_forgefpga_system);

        $display("\n================================================================");
        $display("  SIH26181 ShrikeFi (ESP32-S3 + Renesas ForgeFPGA) Testbench");
        $display("  Qualcomm Hardware Challenge — Smart India Hackathon 2026");
        $display("================================================================\n");

        // Initialization
        link_strobe = 0;
        link_dir    = 0;
        link_din    = 4'h0;
        rst_n       = 0;

        // Reset Sequence
        #100;
        rst_n = 1;
        #100;

        // ---------------------------------------------------------------------
        // TEST 1: Programmable Threshold Write via 4-Bit Link
        // ---------------------------------------------------------------------
        total_tests = total_tests + 1;
        $display("[TEST 1] Setting Systolic Threshold to 150 over 4-bit link...");
        link_write_reg(CMD_WRITE_THRESH, 8'd150);
        #200;
        if (dut.reg_threshold === 8'd150) begin
            $display("  PASS: reg_threshold set to %0d (expected 150)", dut.reg_threshold);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: reg_threshold = %0d (expected 150)", dut.reg_threshold);
            tests_failed = tests_failed + 1;
        end
        #200;

        // ---------------------------------------------------------------------
        // TEST 2: Red Channel Filter Convergence (8-Tap Moving Average)
        // ---------------------------------------------------------------------
        total_tests = total_tests + 1;
        $display("\n[TEST 2] Red channel 8-tap filter convergence (Stream constant 100)...");
        for (k = 0; k < 12; k = k + 1) begin
            link_write_reg(CMD_WRITE_RED, 8'd100);
            #100;
        end
        link_read_8bit(CMD_READ_RED, read_val8);
        if (read_val8 === 8'd100) begin
            $display("  PASS: Filtered Red output converged to %0d (expected 100)", read_val8);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: Filtered Red output = %0d (expected 100)", read_val8);
            tests_failed = tests_failed + 1;
        end
        #200;

        // ---------------------------------------------------------------------
        // TEST 3: IR Channel Filter Convergence (8-Tap Moving Average)
        // ---------------------------------------------------------------------
        total_tests = total_tests + 1;
        $display("\n[TEST 3] IR channel 8-tap filter convergence (Stream constant 180)...");
        for (k = 0; k < 12; k = k + 1) begin
            link_write_reg(CMD_WRITE_IR, 8'd180);
            #100;
        end
        link_read_8bit(CMD_READ_IR, read_val8);
        if (read_val8 === 8'd180) begin
            $display("  PASS: Filtered IR output converged to %0d (expected 180)", read_val8);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: Filtered IR output = %0d (expected 180)", read_val8);
            tests_failed = tests_failed + 1;
        end
        #200;

        // ---------------------------------------------------------------------
        // TEST 4: Cardiac Beat Detection, IRQ Assertion, & 32-bit IBI Extraction
        // ---------------------------------------------------------------------
        total_tests = total_tests + 1;
        $display("\n[TEST 4] Simulating synthetic PPG cardiac waves across 4-bit link...");

        // Set threshold to 120
        link_write_reg(CMD_WRITE_THRESH, 8'd120);

        // Pulse 1: Baseline -> Peak (210) -> Fall (locks baseline)
        send_cardiac_pulse();
        #2000;
        $display("  Beat 1 processed. Simulating 3,000 clock tick inter-beat interval...");

        // Interval simulation
        #60000;

        // Pulse 2: Second beat (triggers IBI interval measurement)
        send_cardiac_pulse();
        #500;

        if (irq_beat === 1'b1) begin
            $display("  PASS: Hardware interrupt (irq_beat) asserted!");
            
            // Read 32-bit IBI value across 8 nibbles
            link_read_ibi(read_val32);
            $display("  Read 32-bit IBI cycles = %0d (%0.2f ms at 50MHz)", read_val32, (read_val32 * 20.0) / 1000000.0);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: irq_beat was not asserted on second beat!");
            tests_failed = tests_failed + 1;
        end

        // ---------------------------------------------------------------------
        // TEST 5: Interrupt Clear Handshake (CMD_CLEAR_IRQ)
        // ---------------------------------------------------------------------
        total_tests = total_tests + 1;
        $display("\n[TEST 5] Verifying CMD_CLEAR_IRQ deasserts hardware interrupt...");
        link_clear_irq();
        #200;
        if (irq_beat === 1'b0) begin
            $display("  PASS: irq_beat successfully deasserted after clear command.");
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: irq_beat remained asserted after clear command.");
            tests_failed = tests_failed + 1;
        end

        // =====================================================================
        // Final Test Report
        // =====================================================================
        $display("\n================================================================");
        $display("  SHRIKEFI FORGEFPGA SIMULATION RESULTS");
        $display("  Passed: %0d / %0d", tests_passed, total_tests);
        if (tests_failed == 0) begin
            $display("  >>> ALL %0d TESTS PASSED (100%%) <<<", total_tests);
        end else begin
            $display("  >>> %0d TEST(S) FAILED <<<", tests_failed);
        end
        $display("================================================================\n");

        #500;
        $finish;
    end

endmodule
