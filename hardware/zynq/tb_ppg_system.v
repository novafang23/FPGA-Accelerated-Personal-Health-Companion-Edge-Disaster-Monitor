`timescale 1ns / 1ps

// tb_ppg_system.v
// Full System Testbench for PPG Accelerator

module tb_ppg_system;

    parameter CLK_PERIOD = 20;   // 50 MHz = 20ns period
    parameter ADDR_WIDTH = 5;
    parameter DATA_WIDTH = 32;

    // ---- Clock & Reset ----
    reg clk, rstn;
    initial clk = 0;
    always #(CLK_PERIOD/2) clk = ~clk;

    // ---- AXI Signals ----
    reg  [ADDR_WIDTH-1:0] awaddr, araddr;
    reg                   awvalid, wvalid, arvalid, bready, rready;
    reg  [DATA_WIDTH-1:0] wdata;
    reg  [3:0]            wstrb;

    wire                  awready, wready, bvalid, arready, rvalid;
    wire [DATA_WIDTH-1:0] rdata;
    wire [1:0]            bresp, rresp;
    wire                  irq_beat;

    // ---- DUT ----
    axi_ppg_accelerator #(
        .C_S_AXI_DATA_WIDTH(DATA_WIDTH),
        .C_S_AXI_ADDR_WIDTH(ADDR_WIDTH),
        .REFRACTORY_CYC(500)
    ) dut (
        .s_axi_aclk    (clk),
        .s_axi_aresetn (rstn),
        .s_axi_awaddr  (awaddr),
        .s_axi_awvalid (awvalid),
        .s_axi_awready (awready),
        .s_axi_wdata   (wdata),
        .s_axi_wstrb   (wstrb),
        .s_axi_wvalid  (wvalid),
        .s_axi_wready  (wready),
        .s_axi_bresp   (bresp),
        .s_axi_bvalid  (bvalid),
        .s_axi_bready  (bready),
        .s_axi_araddr  (araddr),
        .s_axi_arvalid (arvalid),
        .s_axi_arready (arready),
        .s_axi_rdata   (rdata),
        .s_axi_rresp   (rresp),
        .s_axi_rvalid  (rvalid),
        .s_axi_rready  (rready),
        .irq_beat      (irq_beat)
    );

    // AXI4-Lite Bus Functional Model (BFM) Tasks

    // AXI Write: Address and data asserted simultaneously
    task axi_write;
        input [ADDR_WIDTH-1:0] addr;
        input [DATA_WIDTH-1:0] data;
        begin
            @(posedge clk);
            awaddr  <= addr;
            awvalid <= 1'b1;
            wdata   <= data;
            wstrb   <= 4'hF;
            wvalid  <= 1'b1;
            bready  <= 1'b1;

            // Wait for both handshakes to complete
            fork
                begin : aw_handshake
                    @(posedge clk);
                    while (!(awvalid && awready)) @(posedge clk);
                    @(posedge clk);
                    awvalid <= 1'b0;
                end
                begin : w_handshake
                    @(posedge clk);
                    while (!(wvalid && wready)) @(posedge clk);
                    @(posedge clk);
                    wvalid <= 1'b0;
                end
            join

            // Wait for write response
            while (!bvalid) @(posedge clk);
            @(posedge clk);
            bready <= 1'b0;
            @(posedge clk);
        end
    endtask

    // AXI Write with staggered phases (address first, data delayed)
    // Tests the decoupled handshake fix
    task axi_write_staggered;
        input [ADDR_WIDTH-1:0] addr;
        input [DATA_WIDTH-1:0] data;
        input integer data_delay_cycles;
        begin
            @(posedge clk);
            // Assert address first
            awaddr  <= addr;
            awvalid <= 1'b1;
            wvalid  <= 1'b0;
            bready  <= 1'b1;

            // Wait for address handshake
            @(posedge clk);
            while (!(awvalid && awready)) @(posedge clk);
            @(posedge clk);
            awvalid <= 1'b0;

            // Delay before asserting data
            repeat(data_delay_cycles) @(posedge clk);

            // Assert data
            wdata  <= data;
            wstrb  <= 4'hF;
            wvalid <= 1'b1;

            @(posedge clk);
            while (!(wvalid && wready)) @(posedge clk);
            @(posedge clk);
            wvalid <= 1'b0;

            // Wait for write response
            while (!bvalid) @(posedge clk);
            @(posedge clk);
            bready <= 1'b0;
            @(posedge clk);
        end
    endtask

    // AXI Read
    task axi_read;
        input  [ADDR_WIDTH-1:0] addr;
        output [DATA_WIDTH-1:0] data;
        begin
            @(posedge clk);
            araddr  <= addr;
            arvalid <= 1'b1;
            rready  <= 1'b1;

            while (!(arvalid && arready)) @(posedge clk);
            @(posedge clk);
            arvalid <= 1'b0;

            while (!rvalid) @(posedge clk);
            data = rdata;
            @(posedge clk);
            rready <= 1'b0;
            @(posedge clk);
        end
    endtask

    // Synthetic PPG Beat Generator
    // Produces a triangular pulse above baseline to trigger peak detection
    task generate_ppg_beat;
        input integer peak_value;
        input integer baseline;
        input integer rise_samples;
        input integer fall_samples;
        integer i, sample_val;
        begin
            // Rising edge
            for (i = 0; i < rise_samples; i = i + 1) begin
                sample_val = baseline + ((peak_value - baseline) * i) / rise_samples;
                axi_write(5'h00, {24'd0, sample_val[7:0]});
                repeat(50) @(posedge clk);
            end
            // Peak
            axi_write(5'h00, {24'd0, peak_value[7:0]});
            repeat(50) @(posedge clk);
            // Falling edge
            for (i = fall_samples; i > 0; i = i - 1) begin
                sample_val = baseline + ((peak_value - baseline) * i) / fall_samples;
                axi_write(5'h00, {24'd0, sample_val[7:0]});
                repeat(50) @(posedge clk);
            end
            // Return to baseline
            for (i = 0; i < 5; i = i + 1) begin
                axi_write(5'h00, {24'd0, baseline[7:0]});
                repeat(50) @(posedge clk);
            end
        end
    endtask

    // Test Variables
    reg [31:0] read_data;
    integer    test_num;
    integer    tests_passed;
    integer    tests_failed;
    integer    beat_count;

    // Main Test Sequence
    initial begin
        $dumpfile("ppg_system.vcd");
        $dumpvars(0, tb_ppg_system);

        // Initialize all AXI signals
        rstn    = 0;
        awaddr  = 0; awvalid = 0;
        wdata   = 0; wstrb   = 0; wvalid = 0;
        bready  = 0;
        araddr  = 0; arvalid = 0;
        rready  = 0;
        test_num     = 0;
        tests_passed = 0;
        tests_failed = 0;
        beat_count   = 0;

        // Reset sequence
        repeat(20) @(posedge clk);
        rstn = 1;
        repeat(10) @(posedge clk);

        $display("");
        $display("================================================================");
        $display("  SIH26181 PPG Accelerator — Full System Testbench");
        $display("  Qualcomm Hardware Challenge — Smart India Hackathon 2026");
        $display("================================================================");
        $display("");

        // TEST 1: Threshold Register Write & Readback
        test_num = 1;
        $display("[TEST %0d] Threshold register write & readback...", test_num);
        axi_write(5'h0C, 32'h0000_9600);  // threshold = 150 in bits[15:8]
        axi_read(5'h0C, read_data);
        if (read_data[15:8] == 8'd150) begin
            $display("  PASS: Threshold readback = %0d (expected 150)", read_data[15:8]);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: Threshold readback = %0d (expected 150)", read_data[15:8]);
            tests_failed = tests_failed + 1;
        end

        // TEST 2: Staggered AXI Write
        test_num = 2;
        $display("");
        $display("[TEST %0d] Staggered AXI write (addr first, data 5 cycles later)...", test_num);
        axi_write_staggered(5'h0C, 32'h0000_C800, 5);  // threshold = 200
        axi_read(5'h0C, read_data);
        if (read_data[15:8] == 8'd200) begin
            $display("  PASS: Staggered write successful, threshold = %0d", read_data[15:8]);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: Staggered write failed, threshold = %0d (expected 200)", read_data[15:8]);
            tests_failed = tests_failed + 1;
        end

        // TEST 3: Filter Convergence (Constant Input)
        test_num = 3;
        $display("");
        $display("[TEST %0d] Filter convergence with constant input (100)...", test_num);
        // Set threshold back to 120
        axi_write(5'h0C, 32'h0000_7800);  // threshold = 120
        // Push 16 constant samples
        repeat(16) begin
            axi_write(5'h00, 32'h0000_0064);  // value = 100
            repeat(20) @(posedge clk);
        end
        axi_read(5'h04, read_data);
        $display("  Filtered output = %0d", read_data[7:0]);
        if (read_data[7:0] >= 90 && read_data[7:0] <= 110) begin
            $display("  PASS: Filter converged to ~100");
            tests_passed = tests_passed + 1;
        end else begin
            $display("  WARN: Filter value %0d outside expected range [90,110]", read_data[7:0]);
            tests_failed = tests_failed + 1;
        end

        // TEST 4: IR Channel Operation
        test_num = 4;
        $display("");
        $display("[TEST %0d] IR channel write & filter...", test_num);
        repeat(16) begin
            axi_write(5'h10, 32'h0000_00B4);  // IR value = 180
            repeat(20) @(posedge clk);
        end
        axi_read(5'h14, read_data);
        $display("  IR filtered output = %0d", read_data[7:0]);
        if (read_data[7:0] >= 170 && read_data[7:0] <= 190) begin
            $display("  PASS: IR filter converged to ~180");
            tests_passed = tests_passed + 1;
        end else begin
            $display("  WARN: IR filter value %0d outside expected range", read_data[7:0]);
            tests_failed = tests_failed + 1;
        end

        // TEST 5: Beat Detection & IBI Measurement
        test_num = 5;
        $display("");
        $display("[TEST %0d] Beat detection with synthetic PPG pulses...", test_num);
        axi_write(5'h0C, 32'h0000_7800);  // threshold = 120

        // Generate 3 heartbeats — first should be suppressed by first_beat_seen
        begin : beat_test_block
            integer b;
            for (b = 0; b < 3; b = b + 1) begin
                generate_ppg_beat(200, 60, 6, 10);

                // Check beat flag
                axi_read(5'h0C, read_data);
                if (read_data[0]) begin
                    beat_count = beat_count + 1;
                    axi_read(5'h08, read_data);
                    $display("  Beat #%0d detected, IBI = %0d cycles", beat_count, read_data);

                    // Clear beat flag
                    axi_write(5'h0C, 32'h0000_7801);
                end else begin
                    $display("  Pulse %0d: no beat_flag (expected for 1st beat or sub-threshold)", b+1);
                end

                // Inter-beat gap
                repeat(2000) @(posedge clk);
            end
        end

        if (beat_count == 2) begin
            $display("  PASS: %0d beat(s) detected (expected 2: pulse 1 suppressed by first_beat_seen)", beat_count);
            tests_passed = tests_passed + 1;
        end else begin
            $display("  WARN: expected exactly 2 beats (got %0d) -- may need threshold tuning, or a beat is being spuriously double/under-counted", beat_count);
            tests_failed = tests_failed + 1;
        end

        // TEST 6: Write-1-to-Clear Beat Flag
        test_num = 6;
        $display("");
        $display("[TEST %0d] Write-1-to-Clear beat_flag verification...", test_num);
        axi_read(5'h0C, read_data);
        if (read_data[0] == 1'b0) begin
            $display("  PASS: beat_flag is cleared after W1C");
            tests_passed = tests_passed + 1;
        end else begin
            $display("  FAIL: beat_flag still set after W1C");
            tests_failed = tests_failed + 1;
        end

        // Summary
        $display("");
        $display("================================================================");
        $display("  RESULTS: %0d passed, %0d failed (out of %0d tests)",
                 tests_passed, tests_failed, tests_passed + tests_failed);
        if (tests_failed == 0)
            $display("  >>> ALL TESTS PASSED <<<");
        else
            $display("  >>> SOME TESTS FAILED <<<");
        $display("================================================================");
        $display("");

        #1000;
        $finish;
    end

    // ---- Beat Interrupt Monitor ----
    always @(posedge irq_beat) begin
        $display("  [IRQ] Beat pulse at time %0t ns", $time);
    end

    // ---- Simulation Timeout Watchdog ----
    initial begin
        #100_000_000;  // 100ms max simulation time
        $display("");
        $display("  TIMEOUT: Simulation exceeded 100ms limit");
        $finish;
    end

endmodule
