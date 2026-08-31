/****** macros.vh ******/
`define  BYTE                        8
// MAIN FSM STATES //
`define S_IDLE                  5'd0 
`define S_PWR_OFF               5'd1
`define S_FORCE_BG              5'd2
`define S_PRE_LOAD              5'd3
`define S_LOAD_BITSTREAM        5'd4
`define S_LOAD_INIT             5'd5
`define S_FUNC_MODE             5'd6
`define S_RETENTION_SEQ_START   5'd7
`define S_RST_MODE              5'd8
`define S_SLEEP_MODE            5'd9

`define UNKNOWN                      2
`define RISE                         0
`define FALL                         1

/****** bram\dual_port_bram.v ******/
`timescale 1ns/1ps
module dual_port_bram    (
    input wire          i_por,
    input wire          i_web,            
    input wire          i_wclk,           
    input wire          i_wclk_inv,       
    input wire          i_wclken,         
    input wire  [8:0]   i_waddr ,         
    input wire  [7:0]   i_din ,           
    input wire          i_reb,            
    input wire          i_rclk,           
    input wire          i_rclk_inv,       
    input wire          i_rclken,         
    input wire  [8:0]   i_raddr ,         
    output  reg  [7:0]   o_dout,          
    input wire [1:0]    i_ratio,               
    input wire          i_pd,                  
    input wire          i_flag_reset,
    input wire [4095:0] i_config_data, 
    input wire          i_sleep_mode,
    input wire          i_config_done
);
    reg     [7:0]   i_reg_ram [0:511];
    reg     [7:0]   i_reg_latched_data ;
    initial         i_reg_latched_data    = 8'h00;
    localparam BRAM_CONFIG_512x8    = 2'b00;
    localparam BRAM_CONFIG_1024x4   = 2'b01;
    localparam BRAM_CONFIG_2048x2   = 2'b10;
    localparam BRAM_CONFIG_4096x1   = 2'b11;
    wire    w_nwclk;
    wire    w_nrclk;
    wire w_bram_wclk;
    wire w_bram_rclk;
    not inv_1 (w_nwclk, i_wclk);
    not inv_2 (w_nrclk, i_rclk);
    bus_mux #(.width(1)) mux_1  (  .i_por    (i_por        ), 
                               .a      (i_wclk       ),
                               .b      (w_nwclk      ),
                               .sel    (i_wclk_inv   ),
                               .out    (w_bram_wclk  )
                            );
    bus_mux #(.width(1)) mux_2  (   .i_por    (i_por        ),
                                .a      (i_rclk       ),
                                .b      (w_nrclk      ),
                                .sel    (i_rclk_inv   ),
                                .out    (w_bram_rclk  )
                            );
integer i, j;
    initial begin
        for(i = 0; i < 512; i = i + 1) begin
            for(j = 0; j < 8; j = j + 1) begin
                i_reg_ram[i][j] = i_config_data[i*8+j];
            end
        end
    end
always @(negedge i_por)
    begin
        for(i = 0; i < 512; i = i + 1)
                begin
                    i_reg_ram[i] = 8'h00;
                end
        o_dout            <=  8'h00;
        i_reg_latched_data    <=  8'h00;          
    end
always @(posedge i_config_done)
begin
    o_dout            <=  8'h00;
    i_reg_latched_data    <=  8'h00;          
end
always @(posedge i_flag_reset)
    begin
        for(i = 0; i < 512; i = i + 1) begin
            for(j = 0; j < 8; j = j + 1) begin
                i_reg_ram[i][j] = i_config_data[i*8+j];
            end
        end
    o_dout            <=  8'h00;
    i_reg_latched_data    <=  8'h00;     
end
    always @(posedge w_bram_wclk or posedge i_pd)
        begin
            if(i_por & ~i_sleep_mode)
                begin
                    if(i_pd)
                        begin
                        end
                    else
                        begin
                            if(!i_web && !i_wclken)
                                begin
                                    case (i_ratio)
                                        BRAM_CONFIG_512x8:
                                            begin
                                                i_reg_ram[i_waddr] <= i_din;
                                            end
                                        BRAM_CONFIG_1024x4:
                                            begin
                                                if(i_din[7] == 1'b1)
                                                    i_reg_ram[i_waddr][7:4] <= i_din[3:0];
                                                else
                                                    i_reg_ram[i_waddr][3:0] <= i_din[3:0];
                                            end
                                        BRAM_CONFIG_2048x2:
                                            begin
                                                if(i_din[7:6] == 2'b11)
                                                    i_reg_ram[i_waddr][7:6] <= i_din[1:0];
                                                else if (i_din[7:6] == 2'b10)
                                                    i_reg_ram[i_waddr][5:4] <= i_din[1:0];
                                                else if (i_din[7:6] == 2'b01)
                                                    i_reg_ram[i_waddr][3:2] <= i_din[1:0];
                                                else
                                                    i_reg_ram[i_waddr][1:0] <= i_din[1:0];
                                            end                             
                                        BRAM_CONFIG_4096x1:
                                            begin
                                                if(i_din[7:5] == 3'b111)
                                                    i_reg_ram[i_waddr][7] <= i_din[0];
                                                else if (i_din[7:5] == 3'b110)
                                                    i_reg_ram[i_waddr][6] <= i_din[0];
                                                else if (i_din[7:5] == 3'b101)
                                                    i_reg_ram[i_waddr][5] <= i_din[0];
                                                else if (i_din[7:5] == 3'b100)
                                                    i_reg_ram[i_waddr][4] <= i_din[0];
                                                else if (i_din[7:5] == 3'b011)
                                                    i_reg_ram[i_waddr][3] <= i_din[0];
                                                else if (i_din[7:5] == 3'b010)
                                                    i_reg_ram[i_waddr][2] <= i_din[0];
                                                else if (i_din[7:5] == 3'b001)
                                                    i_reg_ram[i_waddr][1] <= i_din[0];
                                                else
                                                    i_reg_ram[i_waddr][0] <= i_din[0];
                                            end                             
                                    endcase
                                end
                        end
                end
        end
    always @(posedge w_bram_rclk or posedge i_pd)
        begin
            if(i_por & ~i_sleep_mode)
                begin
                    if(i_pd)
                        begin
                            o_dout            <= 8'h00;
                        end
                    else
                        begin
                            if(!i_reb && !i_rclken)
                                begin
                                    case (i_ratio)
                                        BRAM_CONFIG_512x8:
                                            begin
                                                o_dout            <= i_reg_ram[i_raddr];
                                                i_reg_latched_data    <= i_reg_ram[i_raddr];
                                            end
                                        BRAM_CONFIG_1024x4:
                                            begin
                                                if(i_din[4] == 1'b1)
                                                    begin
                                                        o_dout            <=  {4'b0000,i_reg_ram[i_raddr][7:4]};
                                                        i_reg_latched_data    <=  {4'b0000, i_reg_ram[i_raddr][7:4]};
                                                    end
                                                else
                                                    begin
                                                        o_dout            <=  {4'b0000, i_reg_ram[i_raddr][3:0]};
                                                        i_reg_latched_data    <=  {4'b0000, i_reg_ram[i_raddr][3:0]};
                                                    end
                                            end
                                        BRAM_CONFIG_2048x2:
                                            begin
                                                if(i_din[4:3] == 2'b11)
                                                    begin
                                                        o_dout            <=  {6'b000000,i_reg_ram[i_raddr][7:6]};
                                                        i_reg_latched_data    <=  {6'b000000, i_reg_ram[i_raddr][7:6]};
                                                    end
                                                else if (i_din[4:3] == 2'b10)
                                                    begin
                                                        o_dout            <=  {6'b000000,i_reg_ram[i_raddr][5:4]};
                                                        i_reg_latched_data    <=  {6'b000000, i_reg_ram[i_raddr][5:4]};
                                                    end
                                                else if (i_din[4:3] == 2'b01)
                                                    begin
                                                        o_dout            <=  {6'b000000,i_reg_ram[i_raddr][3:2]};
                                                        i_reg_latched_data    <=  {6'b000000, i_reg_ram[i_raddr][3:2]};
                                                    end
                                                else
                                                    begin
                                                        o_dout            <=  {6'b000000,i_reg_ram[i_raddr][1:0]};
                                                        i_reg_latched_data    <=  {6'b000000, i_reg_ram[i_raddr][1:0]};
                                                    end
                                            end                             
                                        BRAM_CONFIG_4096x1:
                                            begin
                                                if(i_din[4:2] == 3'b111)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][7]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][7]};
                                                    end 
                                                else if (i_din[4:2] == 3'b110)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][6]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][6]};
                                                    end
                                                else if (i_din[4:2] == 3'b101)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][5]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][5]};
                                                    end
                                                else if (i_din[4:2] == 3'b100)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][4]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][4]};
                                                    end
                                                else if (i_din[4:2] == 3'b011)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][3]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][3]};
                                                    end
                                                else if (i_din[4:2] == 3'b010)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][2]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][2]};
                                                    end
                                                else if (i_din[4:2] == 3'b001)
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][1]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][1]};
                                                    end
                                                else
                                                    begin
                                                        o_dout            <=  {7'b0000000,i_reg_ram[i_raddr][0]};
                                                        i_reg_latched_data    <=  {7'b0000000,i_reg_ram[i_raddr][0]};
                                                    end
                                            end                             
                                    endcase
                                end
                            else
                                begin
                                    o_dout            <=  o_dout;
                                    i_reg_latched_data    <=  i_reg_latched_data;
                                end
                        end
                end
            else
                if(~i_por) begin
                    o_dout            <=  8'h00;
                    i_reg_latched_data    <=  8'h00;     
                end      
        end
    always @(i_din[4] or i_din[3] or i_din[2] or i_ratio)
        begin
            if(i_por & ~i_sleep_mode)
                begin
                    if(!i_pd)
                        begin
                            if(!i_reb && !i_rclken)
                                begin
                                    case (i_ratio)
                                                BRAM_CONFIG_1024x4:
                                                    begin
                                                        if(i_din[4] == 1'b1)
                                                            o_dout <= i_reg_latched_data[7:4];
                                                        else
                                                            o_dout <= i_reg_latched_data[3:0];
                                                    end
                                                BRAM_CONFIG_2048x2:
                                                    begin
                                                        if(i_din[4:3] == 2'b11)
                                                            o_dout <= i_reg_latched_data[7:6];
                                                        else if (i_din[4:3] == 2'b10)
                                                            o_dout <= i_reg_latched_data[5:4];
                                                        else if (i_din[4:3] == 2'b01)
                                                            o_dout <= i_reg_latched_data[3:2];
                                                        else
                                                            o_dout <= i_reg_latched_data[1:0];
                                                    end                             
                                                BRAM_CONFIG_4096x1:
                                                    begin
                                                        if(i_din[4:2] == 3'b111)
                                                            o_dout <= i_reg_latched_data[7];
                                                        else if (i_din[4:2] == 3'b110)
                                                            o_dout <= i_reg_latched_data[6];
                                                        else if (i_din[4:2] == 3'b101)
                                                            o_dout <= i_reg_latched_data[5];
                                                        else if (i_din[4:2] == 3'b100)
                                                            o_dout <= i_reg_latched_data[4];
                                                        else if (i_din[4:2] == 3'b011)
                                                            o_dout <= i_reg_latched_data[3];
                                                        else if (i_din[4:2] == 3'b010)
                                                            o_dout <= i_reg_latched_data[2];
                                                        else if (i_din[4:2] == 3'b001)
                                                            o_dout <= i_reg_latched_data[1];
                                                        else
                                                            o_dout <= i_reg_latched_data[0];
                                                    end                             
                                    endcase
                            end
                        end
                end
            else
                begin
                    if(~i_por) begin
                        o_dout            <=  8'h00;
                        i_reg_latched_data    <=  8'h00;     
                    end                    
                end
        end
endmodule

/****** bram\dual_port_hbram.v ******/
`timescale 1ns/1ps
module dual_port_hbram   (
    input wire                i_vdd_to_bram_0,      
    input wire                i_bram_north_write_clk,  
    input wire                i_bram_north_read_clk,   
    input wire                i_bram0_web_0,        
    input wire  [1:0]         i_bram0_1_wclken_0,        
    input wire  [1:0]         i_bram0_1_wclk_inv_0,      
    input wire  [8:0]         i_bram0_write_addr_0,   
    input wire  [8:0]         i_bram1_write_addr_0,    
    input wire  [7:0]         i_bram0_data_in_0,         
    input wire                i_bram1_web_0,                              
    input wire  [7:0]         i_bram1_data_in_0,         
    input wire                i_bram2_web_0,             
    input wire  [1:0]         i_bram2_3_wclken_0,        
    input wire  [1:0]         i_bram2_3_wclk_inv_0,      
    input wire  [8:0]         i_bram2_write_addr_0,      
    input wire  [8:0]         i_bram3_write_addr_0,  
    input wire  [7:0]         i_bram2_data_in_0,         
    input wire                i_bram3_web_0,                            
    input wire  [7:0]         i_bram3_data_in_0,         
    input wire  [1:0]         i_bram0_1_reb_0,          
    input wire  [1:0]         i_bram0_1_rclken_0,        
    input wire  [1:0]         i_bram0_1_rclk_inv_0,      
    input wire  [8:0]         i_bram0_read_addr_0,     
    input wire  [8:0]         i_bram1_read_addr_0,                                
    input wire  [1:0]         i_bram2_3_reb_0,           
    input wire  [1:0]         i_bram2_3_rclken_0,        
    input wire  [1:0]         i_bram2_3_rclk_inv_0,      
    input wire  [8:0]         i_bram2_read_addr_0,     
    input wire  [8:0]         i_bram3_read_addr_0,                              
    input wire  [1:0]         i_bram0_ratio_0,  
    input wire  [1:0]         i_bram1_ratio_0,        
    input wire  [1:0]         i_bram0_1_pd_0,                              
    input wire  [1:0]         i_bram2_ratio_0,                            
    input wire  [1:0]         i_bram3_ratio_0,        
    input wire  [1:0]         i_bram2_3_pd_0,                            
    output wire [7:0]         o_bram0_data_out_0,       
    output wire [7:0]         o_bram1_data_out_0,        
    output wire [7:0]         o_bram2_data_out_0,        
    output wire [7:0]         o_bram3_data_out_0,    
    input wire                i_vdd_to_bram_1,      
    input wire                i_bram_south_write_clk,   
    input wire                i_bram_south_read_clk,    
    input wire                i_bram0_web_1,              
    input wire  [1:0]         i_bram0_1_wclken_1,         
    input wire  [1:0]         i_bram0_1_wclk_inv_1,       
    input wire  [8:0]         i_bram0_write_addr_1,     
    input wire  [8:0]         i_bram1_write_addr_1,     
    input wire  [7:0]         i_bram0_data_in_1,          
    input wire                i_bram1_web_1,                             
    input wire  [7:0]         i_bram1_data_in_1,          
    input wire                i_bram2_web_1,              
    input wire  [1:0]         i_bram2_3_wclken_1,         
    input wire  [1:0]         i_bram2_3_wclk_inv_1,       
    input wire  [8:0]         i_bram2_write_addr_1,     
    input wire  [8:0]         i_bram3_write_addr_1,    
    input wire  [7:0]         i_bram2_data_in_1,          
    input wire                i_bram3_web_1,                           
    input wire  [7:0]         i_bram3_data_in_1,          
    input wire  [1:0]         i_bram0_1_reb_1,            
    input wire  [1:0]         i_bram0_1_rclken_1,         
    input wire  [1:0]         i_bram0_1_rclk_inv_1,       
    input wire  [8:0]         i_bram0_read_addr_1,     
    input wire  [8:0]         i_bram1_read_addr_1,           
    input wire  [1:0]         i_bram2_3_reb_1,            
    input wire  [1:0]         i_bram2_3_rclken_1,         
    input wire  [1:0]         i_bram2_3_rclk_inv_1,       
    input wire  [8:0]         i_bram2_read_addr_1,       
    input wire  [8:0]         i_bram3_read_addr_1,          
    input wire  [1:0]         i_bram0_ratio_1,  
    input wire  [1:0]         i_bram1_ratio_1,          
    input wire  [1:0]         i_bram0_1_pd_1,                                  
    input wire  [1:0]         i_bram2_ratio_1,                               
    input wire  [1:0]         i_bram3_ratio_1,         
    input wire  [1:0]         i_bram2_3_pd_1,                                 
    output wire [7:0]         o_bram0_data_out_1,         
    output wire [7:0]         o_bram1_data_out_1,         
    output wire [7:0]         o_bram2_data_out_1,         
    output wire [7:0]         o_bram3_data_out_1,
    input wire              i_reset_mode,
    input wire              i_bram_keep_reg,
    input wire  [1:0]       i_pwr_enable_reg,
    input wire [32767:0]    i_config_data,
    input wire              i_sleep_mode,
    input wire              i_config_done
);
wire [16383:0] w_config_data_bram0;
wire [16383:0] w_config_data_bram1;
wire [8:0] w_bram0_write_addr_0, w_bram0_read_addr_0,
           w_bram1_write_addr_0, w_bram1_read_addr_0,
           w_bram2_write_addr_0, w_bram2_read_addr_0,
           w_bram3_write_addr_0, w_bram3_read_addr_0,
           w_bram0_write_addr_1, w_bram0_read_addr_1,
           w_bram1_write_addr_1, w_bram1_read_addr_1,
           w_bram2_write_addr_1, w_bram2_read_addr_1,
           w_bram3_write_addr_1, w_bram3_read_addr_1;
wire [1:0] w_bram0_ratio_0, w_bram1_ratio_0, w_bram2_ratio_0, w_bram3_ratio_0,
           w_bram0_ratio_1, w_bram1_ratio_1, w_bram2_ratio_1, w_bram3_ratio_1;
wire w_bram0_wclken_0;
wire w_bram0_wclk_inv_0;
wire w_bram0_reb_0;
wire w_bram0_rclken_0;
wire w_bram0_rclk_inv_0;
wire w_bram0_pd_0;
wire w_bram1_wclken_0;
wire w_bram1_wclk_inv_0;
wire w_bram1_reb_0;
wire w_bram1_rclken_0;
wire w_bram1_rclk_inv_0;
wire w_bram1_pd_0;
wire w_bram2_wclken_0;
wire w_bram2_wclk_inv_0;
wire w_bram2_reb_0;
wire w_bram2_rclken_0;
wire w_bram2_rclk_inv_0;
wire w_bram2_pd_0;
wire w_bram3_wclken_0;
wire w_bram3_wclk_inv_0;
wire w_bram3_reb_0;
wire w_bram3_rclken_0;
wire w_bram3_rclk_inv_0;
wire w_bram3_pd_0;
wire w_bram0_wclken_1;
wire w_bram0_wclk_inv_1;
wire w_bram0_reb_1;
wire w_bram0_rclken_1;
wire w_bram0_rclk_inv_1;
wire w_bram0_pd_1;
wire w_bram1_wclken_1;
wire w_bram1_wclk_inv_1;
wire w_bram1_reb_1;
wire w_bram1_rclken_1;
wire w_bram1_rclk_inv_1;
wire w_bram1_pd_1;
wire w_bram2_wclken_1;
wire w_bram2_wclk_inv_1;
wire w_bram2_reb_1;
wire w_bram2_rclken_1;
wire w_bram2_rclk_inv_1;
wire w_bram2_pd_1;
wire w_bram3_wclken_1;
wire w_bram3_wclk_inv_1;
wire w_bram3_reb_1;
wire w_bram3_rclken_1;
wire w_bram3_rclk_inv_1;
wire w_bram3_pd_1;
wire [7:0] w_bram0_data_in_0;
wire [7:0] w_bram1_data_in_0;
wire [7:0] w_bram2_data_in_0;
wire [7:0] w_bram3_data_in_0;
wire [7:0] w_bram0_data_in_1;
wire [7:0] w_bram1_data_in_1;
wire [7:0] w_bram2_data_in_1;
wire [7:0] w_bram3_data_in_1;
assign w_config_data_bram0 = i_config_data [16383:0    ];
assign w_config_data_bram1 = i_config_data [32767:16384];
reg            r_reg_BRAM_reset;
integer i;
initial begin r_reg_BRAM_reset = 2'b00; end 
always @(posedge i_reset_mode) begin
    if(i_bram_keep_reg) begin
        r_reg_BRAM_reset = 1'b0;
    end
    else begin
        r_reg_BRAM_reset = 1'b1;
    end
end
always @(negedge i_reset_mode) begin
    r_reg_BRAM_reset = 0;
end
wire w_config_and_keep0;
wire w_config_and_keep1;
assign w_config_and_keep0 = i_config_done;
assign w_config_and_keep1 = i_config_done;
and and1(w_vdd_to_bram_0_post,i_pwr_enable_reg[0],i_vdd_to_bram_0);
and and2(w_vdd_to_bram_1_post,i_pwr_enable_reg[1],i_vdd_to_bram_0);
assign #2.78 w_bram0_data_in_0[0] = i_bram0_data_in_0[0];
assign #3.27 w_bram0_data_in_0[1] = i_bram0_data_in_0[1];
assign #3.40 w_bram0_data_in_0[2] = i_bram0_data_in_0[2];
assign #3.59 w_bram0_data_in_0[3] = i_bram0_data_in_0[3];
assign #3.28 w_bram0_data_in_0[4] = i_bram0_data_in_0[4];
assign #2.75 w_bram0_data_in_0[5] = i_bram0_data_in_0[5];
assign #2.94 w_bram0_data_in_0[6] = i_bram0_data_in_0[6];
assign #3.01 w_bram0_data_in_0[7] = i_bram0_data_in_0[7];
assign #3.10 w_bram0_write_addr_0[0]  = i_bram0_write_addr_0[0];
assign #2.96 w_bram0_write_addr_0[1]  = i_bram0_write_addr_0[1];
assign #3.06 w_bram0_write_addr_0[2]  = i_bram0_write_addr_0[2];
assign #2.64 w_bram0_write_addr_0[3]  = i_bram0_write_addr_0[3];
assign #2.82 w_bram0_write_addr_0[4]  = i_bram0_write_addr_0[4];
assign #2.92 w_bram0_write_addr_0[5]  = i_bram0_write_addr_0[5];
assign #2.80 w_bram0_write_addr_0[6]  = i_bram0_write_addr_0[6];
assign #2.82 w_bram0_write_addr_0[7]  = i_bram0_write_addr_0[7];
assign #2.85 w_bram0_write_addr_0[8]  = i_bram0_write_addr_0[8];
assign #1.65 w_bram0_wclken_0         = i_bram0_1_wclken_0[0];
assign #1.20 w_bram0_wclk_inv_0       = i_bram0_1_wclk_inv_0[0]; 
assign #2.69 w_bram0_reb_0            = i_bram0_1_reb_0[0];
assign #1.39 w_bram0_rclken_0         = i_bram0_1_rclken_0[0];
assign #1.15 w_bram0_rclk_inv_0       = i_bram0_1_rclk_inv_0[0]; 
assign #2.25 w_bram0_read_addr_0[0]   = i_bram0_read_addr_0[0];
assign #2.52 w_bram0_read_addr_0[1]   = i_bram0_read_addr_0[1];
assign #2.56 w_bram0_read_addr_0[2]   = i_bram0_read_addr_0[2];
assign #2.56 w_bram0_read_addr_0[3]   = i_bram0_read_addr_0[3];
assign #2.64 w_bram0_read_addr_0[4]   = i_bram0_read_addr_0[4];
assign #2.71 w_bram0_read_addr_0[5]   = i_bram0_read_addr_0[5];
assign #2.65 w_bram0_read_addr_0[6]   = i_bram0_read_addr_0[6];
assign #2.65 w_bram0_read_addr_0[7]   = i_bram0_read_addr_0[7];
assign #2.47 w_bram0_read_addr_0[8]   = i_bram0_read_addr_0[8];
assign #2.70 w_bram0_ratio_0[0]       = i_bram0_ratio_0[0];
assign #3.36 w_bram0_ratio_0[1]       = i_bram0_ratio_0[1];
assign #5.97 w_bram0_pd_0             = i_bram0_1_pd_0[0]; 
assign #2.68 w_bram1_data_in_0[0] = i_bram1_data_in_0[0];
assign #2.50 w_bram1_data_in_0[1] = i_bram1_data_in_0[1];
assign #2.36 w_bram1_data_in_0[2] = i_bram1_data_in_0[2];
assign #2.72 w_bram1_data_in_0[3] = i_bram1_data_in_0[3];
assign #2.95 w_bram1_data_in_0[4] = i_bram1_data_in_0[4];
assign #2.60 w_bram1_data_in_0[5] = i_bram1_data_in_0[5];
assign #2.55 w_bram1_data_in_0[6] = i_bram1_data_in_0[6];
assign #2.52 w_bram1_data_in_0[7] = i_bram1_data_in_0[7];
assign #3.19 w_bram1_write_addr_0[0]  = i_bram1_write_addr_0[0];
assign #3.12 w_bram1_write_addr_0[1]  = i_bram1_write_addr_0[1];
assign #3.06 w_bram1_write_addr_0[2]  = i_bram1_write_addr_0[2];
assign #2.99 w_bram1_write_addr_0[3]  = i_bram1_write_addr_0[3];
assign #2.76 w_bram1_write_addr_0[4]  = i_bram1_write_addr_0[4];
assign #3.16 w_bram1_write_addr_0[5]  = i_bram1_write_addr_0[5];
assign #2.70 w_bram1_write_addr_0[6]  = i_bram1_write_addr_0[6];
assign #2.75 w_bram1_write_addr_0[7]  = i_bram1_write_addr_0[7];
assign #2.88 w_bram1_write_addr_0[8]  = i_bram1_write_addr_0[8];
assign #2.23 w_bram1_wclken_0         = i_bram0_1_wclken_0[1];
assign #0.76 w_bram1_wclk_inv_0       = i_bram0_1_wclk_inv_0[1]; 
assign #2.61 w_bram1_reb_0            = i_bram0_1_reb_0[1];
assign #2.05 w_bram1_rclken_0         = i_bram0_1_rclken_0[1];
assign #0.91 w_bram1_rclk_inv_0       = i_bram0_1_rclk_inv_0[1]; 
assign #2.31 w_bram1_read_addr_0[0]   = i_bram1_read_addr_0[0];
assign #2.56 w_bram1_read_addr_0[1]   = i_bram1_read_addr_0[1];
assign #2.55 w_bram1_read_addr_0[2]   = i_bram1_read_addr_0[2];
assign #2.76 w_bram1_read_addr_0[3]   = i_bram1_read_addr_0[3];
assign #2.70 w_bram1_read_addr_0[4]   = i_bram1_read_addr_0[4];
assign #2.36 w_bram1_read_addr_0[5]   = i_bram1_read_addr_0[5];
assign #2.60 w_bram1_read_addr_0[6]   = i_bram1_read_addr_0[6];
assign #2.69 w_bram1_read_addr_0[7]   = i_bram1_read_addr_0[7];
assign #2.36 w_bram1_read_addr_0[8]   = i_bram1_read_addr_0[8];
assign #2.48 w_bram1_ratio_0[0]       = i_bram1_ratio_0[0];
assign #2.91 w_bram1_ratio_0[1]       = i_bram1_ratio_0[1];
assign #5.97 w_bram1_pd_0             = i_bram0_1_pd_0[1]; 
assign #3.26 w_bram2_data_in_0[0] = i_bram2_data_in_0[0];
assign #2.62 w_bram2_data_in_0[1] = i_bram2_data_in_0[1];
assign #2.51 w_bram2_data_in_0[2] = i_bram2_data_in_0[2];
assign #2.78 w_bram2_data_in_0[3] = i_bram2_data_in_0[3];
assign #3.01 w_bram2_data_in_0[4] = i_bram2_data_in_0[4];
assign #3.02 w_bram2_data_in_0[5] = i_bram2_data_in_0[5];
assign #2.99 w_bram2_data_in_0[6] = i_bram2_data_in_0[6];
assign #2.93 w_bram2_data_in_0[7] = i_bram2_data_in_0[7];
assign #2.94 w_bram2_write_addr_0[0]  = i_bram2_write_addr_0[0];
assign #3.00 w_bram2_write_addr_0[1]  = i_bram2_write_addr_0[1];
assign #3.19 w_bram2_write_addr_0[2]  = i_bram2_write_addr_0[2];
assign #3.17 w_bram2_write_addr_0[3]  = i_bram2_write_addr_0[3];
assign #3.21 w_bram2_write_addr_0[4]  = i_bram2_write_addr_0[4];
assign #3.20 w_bram2_write_addr_0[5]  = i_bram2_write_addr_0[5];
assign #3.05 w_bram2_write_addr_0[6]  = i_bram2_write_addr_0[6];
assign #2.45 w_bram2_write_addr_0[7]  = i_bram2_write_addr_0[7];
assign #2.95 w_bram2_write_addr_0[8]  = i_bram2_write_addr_0[8];
assign #2.03 w_bram2_wclken_0         = i_bram2_3_wclken_0[0];
assign #0.81 w_bram2_wclk_inv_0       = i_bram2_3_wclk_inv_0[0]; 
assign #2.53 w_bram2_reb_0            = i_bram2_3_reb_0[0];
assign #2.08 w_bram2_rclken_0         = i_bram2_3_rclken_0[0];
assign #0.68 w_bram2_rclk_inv_0       = i_bram2_3_rclk_inv_0[0]; 
assign #2.73 w_bram2_read_addr_0[0]   = i_bram2_read_addr_0[0];
assign #2.24 w_bram2_read_addr_0[1]   = i_bram2_read_addr_0[1];
assign #2.30 w_bram2_read_addr_0[2]   = i_bram2_read_addr_0[2];
assign #2.65 w_bram2_read_addr_0[3]   = i_bram2_read_addr_0[3];
assign #2.61 w_bram2_read_addr_0[4]   = i_bram2_read_addr_0[4];
assign #2.74 w_bram2_read_addr_0[5]   = i_bram2_read_addr_0[5];
assign #2.76 w_bram2_read_addr_0[6]   = i_bram2_read_addr_0[6];
assign #2.73 w_bram2_read_addr_0[7]   = i_bram2_read_addr_0[7];
assign #2.71 w_bram2_read_addr_0[8]   = i_bram2_read_addr_0[8];
assign #2.67 w_bram2_ratio_0[0]       = i_bram2_ratio_0[0];
assign #2.91 w_bram2_ratio_0[1]       = i_bram2_ratio_0[1];
assign #5.97 w_bram2_pd_0             = i_bram2_3_pd_0[0]; 
assign #3.28 w_bram3_data_in_0[0] = i_bram3_data_in_0[0];
assign #2.52 w_bram3_data_in_0[1] = i_bram3_data_in_0[1];
assign #2.94 w_bram3_data_in_0[2] = i_bram3_data_in_0[2];
assign #2.97 w_bram3_data_in_0[3] = i_bram3_data_in_0[3];
assign #2.94 w_bram3_data_in_0[4] = i_bram3_data_in_0[4];
assign #2.83 w_bram3_data_in_0[5] = i_bram3_data_in_0[5];
assign #2.96 w_bram3_data_in_0[6] = i_bram3_data_in_0[6];
assign #2.92 w_bram3_data_in_0[7] = i_bram3_data_in_0[7];
assign #3.16 w_bram3_write_addr_0[0]  = i_bram3_write_addr_0[0];
assign #2.95 w_bram3_write_addr_0[1]  = i_bram3_write_addr_0[1];
assign #3.20 w_bram3_write_addr_0[2]  = i_bram3_write_addr_0[2];
assign #2.99 w_bram3_write_addr_0[3]  = i_bram3_write_addr_0[3];
assign #3.04 w_bram3_write_addr_0[4]  = i_bram3_write_addr_0[4];
assign #3.03 w_bram3_write_addr_0[5]  = i_bram3_write_addr_0[5];
assign #2.97 w_bram3_write_addr_0[6]  = i_bram3_write_addr_0[6];
assign #2.87 w_bram3_write_addr_0[7]  = i_bram3_write_addr_0[7];
assign #2.89 w_bram3_write_addr_0[8]  = i_bram3_write_addr_0[8];
assign #2.10 w_bram3_wclken_0         = i_bram2_3_wclken_0[1];
assign #0.43 w_bram3_wclk_inv_0       = i_bram2_3_wclk_inv_0[1]; 
assign #2.52 w_bram3_reb_0            = i_bram2_3_reb_0[1];
assign #1.97 w_bram3_rclken_0         = i_bram2_3_rclken_0[1];
assign #0.43 w_bram3_rclk_inv_0       = i_bram2_3_rclk_inv_0[1]; 
assign #2.60 w_bram3_read_addr_0[0]   = i_bram3_read_addr_0[0];
assign #2.54 w_bram3_read_addr_0[1]   = i_bram3_read_addr_0[1];
assign #2.61 w_bram3_read_addr_0[2]   = i_bram3_read_addr_0[2];
assign #2.57 w_bram3_read_addr_0[3]   = i_bram3_read_addr_0[3];
assign #2.76 w_bram3_read_addr_0[4]   = i_bram3_read_addr_0[4];
assign #2.71 w_bram3_read_addr_0[5]   = i_bram3_read_addr_0[5];
assign #2.72 w_bram3_read_addr_0[6]   = i_bram3_read_addr_0[6];
assign #2.70 w_bram3_read_addr_0[7]   = i_bram3_read_addr_0[7];
assign #2.73 w_bram3_read_addr_0[8]   = i_bram3_read_addr_0[8];
assign #2.09 w_bram3_ratio_0[0]       = i_bram3_ratio_0[0];
assign #2.78 w_bram3_ratio_0[1]       = i_bram3_ratio_0[1];
assign #1.73 w_bram3_pd_0             = i_bram2_3_pd_0[1]; 
assign #3.05 w_bram0_data_in_1[0] = i_bram0_data_in_1[0];
assign #2.97 w_bram0_data_in_1[1] = i_bram0_data_in_1[1];
assign #2.65 w_bram0_data_in_1[2] = i_bram0_data_in_1[2];
assign #3.06 w_bram0_data_in_1[3] = i_bram0_data_in_1[3];
assign #3.01 w_bram0_data_in_1[4] = i_bram0_data_in_1[4];
assign #2.71 w_bram0_data_in_1[5] = i_bram0_data_in_1[5];
assign #2.76 w_bram0_data_in_1[6] = i_bram0_data_in_1[6];
assign #2.75 w_bram0_data_in_1[7] = i_bram0_data_in_1[7];
assign #2.70 w_bram0_write_addr_1[0]  = i_bram0_write_addr_1[0];
assign #2.84 w_bram0_write_addr_1[1]  = i_bram0_write_addr_1[1];
assign #2.99 w_bram0_write_addr_1[2]  = i_bram0_write_addr_1[2];
assign #3.09 w_bram0_write_addr_1[3]  = i_bram0_write_addr_1[3];
assign #3.00 w_bram0_write_addr_1[4]  = i_bram0_write_addr_1[4];
assign #2.83 w_bram0_write_addr_1[5]  = i_bram0_write_addr_1[5];
assign #2.91 w_bram0_write_addr_1[6]  = i_bram0_write_addr_1[6];
assign #3.17 w_bram0_write_addr_1[7]  = i_bram0_write_addr_1[7];
assign #2.92 w_bram0_write_addr_1[8]  = i_bram0_write_addr_1[8];
assign #2.01 w_bram0_wclken_1         = i_bram0_1_wclken_1[0];
assign #2.66 w_bram0_wclk_inv_1       = i_bram0_1_wclk_inv_1[0]; 
assign #2.49 w_bram0_reb_1            = i_bram0_1_reb_1[0];
assign #1.91 w_bram0_rclken_1         = i_bram0_1_rclken_1[0];
assign #0.88 w_bram0_rclk_inv_1       = i_bram0_1_rclk_inv_1[0]; 
assign #2.55 w_bram0_read_addr_1[0]   = i_bram0_read_addr_1[0];
assign #2.59 w_bram0_read_addr_1[1]   = i_bram0_read_addr_1[1];
assign #2.58 w_bram0_read_addr_1[2]   = i_bram0_read_addr_1[2];
assign #2.54 w_bram0_read_addr_1[3]   = i_bram0_read_addr_1[3];
assign #2.58 w_bram0_read_addr_1[4]   = i_bram0_read_addr_1[4];
assign #2.60 w_bram0_read_addr_1[5]   = i_bram0_read_addr_1[5];
assign #2.56 w_bram0_read_addr_1[6]   = i_bram0_read_addr_1[6];
assign #2.55 w_bram0_read_addr_1[7]   = i_bram0_read_addr_1[7];
assign #2.49 w_bram0_read_addr_1[8]   = i_bram0_read_addr_1[8];
assign #2.70 w_bram0_ratio_1[0]       = i_bram0_ratio_1[0];
assign #2.89 w_bram0_ratio_1[1]       = i_bram0_ratio_1[1];
assign #1.77 w_bram0_pd_1             = i_bram0_1_pd_1[0]; 
assign #2.68 w_bram1_data_in_1[0] = i_bram1_data_in_1[0];         
assign #2.69 w_bram1_data_in_1[1] = i_bram1_data_in_1[1];         
assign #2.72 w_bram1_data_in_1[2] = i_bram1_data_in_1[2];         
assign #2.95 w_bram1_data_in_1[3] = i_bram1_data_in_1[3];         
assign #2.78 w_bram1_data_in_1[4] = i_bram1_data_in_1[4];         
assign #2.86 w_bram1_data_in_1[5] = i_bram1_data_in_1[5];         
assign #2.84 w_bram1_data_in_1[6] = i_bram1_data_in_1[6];         
assign #2.29 w_bram1_data_in_1[7] = i_bram1_data_in_1[7];         
assign #3.16 w_bram1_write_addr_1[0]  = i_bram1_write_addr_1[0];  
assign #3.08 w_bram1_write_addr_1[1]  = i_bram1_write_addr_1[1];  
assign #3.00 w_bram1_write_addr_1[2]  = i_bram1_write_addr_1[2];  
assign #2.55 w_bram1_write_addr_1[3]  = i_bram1_write_addr_1[3];  
assign #2.73 w_bram1_write_addr_1[4]  = i_bram1_write_addr_1[4];  
assign #3.14 w_bram1_write_addr_1[5]  = i_bram1_write_addr_1[5];  
assign #2.96 w_bram1_write_addr_1[6]  = i_bram1_write_addr_1[6];  
assign #3.24 w_bram1_write_addr_1[7]  = i_bram1_write_addr_1[7];  
assign #2.98 w_bram1_write_addr_1[8]  = i_bram1_write_addr_1[8];  
assign #2.17 w_bram1_wclken_1         = i_bram0_1_wclken_1[1];    
assign #1.00 w_bram1_wclk_inv_1       = i_bram0_1_wclk_inv_1[1];  
assign #2.53 w_bram1_reb_1            = i_bram0_1_reb_1[1];       
assign #2.08 w_bram1_rclken_1         = i_bram0_1_rclken_1[1];    
assign #0.86 w_bram1_rclk_inv_1       = i_bram0_1_rclk_inv_1[1];  
assign #2.73 w_bram1_read_addr_1[0]   = i_bram1_read_addr_1[0];   
assign #2.24 w_bram1_read_addr_1[1]   = i_bram1_read_addr_1[1];   
assign #2.30 w_bram1_read_addr_1[2]   = i_bram1_read_addr_1[2];   
assign #2.65 w_bram1_read_addr_1[3]   = i_bram1_read_addr_1[3];   
assign #2.61 w_bram1_read_addr_1[4]   = i_bram1_read_addr_1[4];   
assign #2.74 w_bram1_read_addr_1[5]   = i_bram1_read_addr_1[5];   
assign #2.76 w_bram1_read_addr_1[6]   = i_bram1_read_addr_1[6];   
assign #2.73 w_bram1_read_addr_1[7]   = i_bram1_read_addr_1[7];   
assign #2.71 w_bram1_read_addr_1[8]   = i_bram1_read_addr_1[8];   
assign #2.51 w_bram1_ratio_1[0]       = i_bram1_ratio_1[0];       
assign #2.16 w_bram1_ratio_1[1]       = i_bram1_ratio_1[1];       
assign #5.97 w_bram1_pd_1             = i_bram0_1_pd_1[1];        
assign #2.71 w_bram2_data_in_1[0] = i_bram2_data_in_1[0];         
assign #2.59 w_bram2_data_in_1[1] = i_bram2_data_in_1[1];         
assign #2.86 w_bram2_data_in_1[2] = i_bram2_data_in_1[2];         
assign #2.79 w_bram2_data_in_1[3] = i_bram2_data_in_1[3];         
assign #3.11 w_bram2_data_in_1[4] = i_bram2_data_in_1[4];         
assign #2.50 w_bram2_data_in_1[5] = i_bram2_data_in_1[5];         
assign #2.81 w_bram2_data_in_1[6] = i_bram2_data_in_1[6];         
assign #2.78 w_bram2_data_in_1[7] = i_bram2_data_in_1[7];         
assign #2.88 w_bram2_write_addr_1[0]  = i_bram2_write_addr_1[0];  
assign #2.95 w_bram2_write_addr_1[1]  = i_bram2_write_addr_1[1];  
assign #2.95 w_bram2_write_addr_1[2]  = i_bram2_write_addr_1[2];  
assign #2.95 w_bram2_write_addr_1[3]  = i_bram2_write_addr_1[3];  
assign #3.10 w_bram2_write_addr_1[4]  = i_bram2_write_addr_1[4];  
assign #3.00 w_bram2_write_addr_1[5]  = i_bram2_write_addr_1[5];  
assign #3.16 w_bram2_write_addr_1[6]  = i_bram2_write_addr_1[6];  
assign #2.94 w_bram2_write_addr_1[7]  = i_bram2_write_addr_1[7];  
assign #2.91 w_bram2_write_addr_1[8]  = i_bram2_write_addr_1[8];  
assign #2.11 w_bram2_wclken_1         = i_bram2_3_wclken_1[0];    
assign #0.90 w_bram2_wclk_inv_1       = i_bram2_3_wclk_inv_1[0];  
assign #2.53 w_bram2_reb_1            = i_bram2_3_reb_1[0];       
assign #2.08 w_bram2_rclken_1         = i_bram2_3_rclken_1[0];    
assign #0.74 w_bram2_rclk_inv_1       = i_bram2_3_rclk_inv_1[0];  
assign #2.73 w_bram2_read_addr_1[0]   = i_bram2_read_addr_1[0];   
assign #2.24 w_bram2_read_addr_1[1]   = i_bram2_read_addr_1[1];   
assign #2.30 w_bram2_read_addr_1[2]   = i_bram2_read_addr_1[2];   
assign #2.65 w_bram2_read_addr_1[3]   = i_bram2_read_addr_1[3];   
assign #2.61 w_bram2_read_addr_1[4]   = i_bram2_read_addr_1[4];   
assign #2.74 w_bram2_read_addr_1[5]   = i_bram2_read_addr_1[5];   
assign #2.76 w_bram2_read_addr_1[6]   = i_bram2_read_addr_1[6];   
assign #2.73 w_bram2_read_addr_1[7]   = i_bram2_read_addr_1[7];   
assign #2.71 w_bram2_read_addr_1[8]   = i_bram2_read_addr_1[8];   
assign #2.32 w_bram2_ratio_1[0]       = i_bram2_ratio_1[0];       
assign #2.33 w_bram2_ratio_1[1]       = i_bram2_ratio_1[1];       
assign #5.97 w_bram2_pd_1             = i_bram2_3_pd_1[0];        
assign #2.69 w_bram3_data_in_1[0] = i_bram3_data_in_1[0];         
assign #2.68 w_bram3_data_in_1[1] = i_bram3_data_in_1[1];         
assign #2.52 w_bram3_data_in_1[2] = i_bram3_data_in_1[2];         
assign #2.48 w_bram3_data_in_1[3] = i_bram3_data_in_1[3];         
assign #2.65 w_bram3_data_in_1[4] = i_bram3_data_in_1[4];         
assign #2.44 w_bram3_data_in_1[5] = i_bram3_data_in_1[5];         
assign #2.66 w_bram3_data_in_1[6] = i_bram3_data_in_1[6];         
assign #2.57 w_bram3_data_in_1[7] = i_bram3_data_in_1[7];         
assign #2.86 w_bram3_write_addr_1[0]  = i_bram3_write_addr_1[0];  
assign #2.92 w_bram3_write_addr_1[1]  = i_bram3_write_addr_1[1];  
assign #2.84 w_bram3_write_addr_1[2]  = i_bram3_write_addr_1[2];  
assign #2.74 w_bram3_write_addr_1[3]  = i_bram3_write_addr_1[3];  
assign #2.75 w_bram3_write_addr_1[4]  = i_bram3_write_addr_1[4];  
assign #2.85 w_bram3_write_addr_1[5]  = i_bram3_write_addr_1[5];  
assign #2.70 w_bram3_write_addr_1[6]  = i_bram3_write_addr_1[6];  
assign #2.66 w_bram3_write_addr_1[7]  = i_bram3_write_addr_1[7];  
assign #2.72 w_bram3_write_addr_1[8]  = i_bram3_write_addr_1[8];  
assign #2.00 w_bram3_wclken_1         = i_bram2_3_wclken_1[1];    
assign #0.43 w_bram3_wclk_inv_1       = i_bram2_3_wclk_inv_1[1];  
assign #2.53 w_bram3_reb_1            = i_bram2_3_reb_1[1];       
assign #2.08 w_bram3_rclken_1         = i_bram2_3_rclken_1[1];    
assign #0.56 w_bram3_rclk_inv_1       = i_bram2_3_rclk_inv_1[1];  
assign #2.73 w_bram3_read_addr_1[0]   = i_bram3_read_addr_1[0];   
assign #2.24 w_bram3_read_addr_1[1]   = i_bram3_read_addr_1[1];   
assign #2.30 w_bram3_read_addr_1[2]   = i_bram3_read_addr_1[2];   
assign #2.65 w_bram3_read_addr_1[3]   = i_bram3_read_addr_1[3];   
assign #2.61 w_bram3_read_addr_1[4]   = i_bram3_read_addr_1[4];   
assign #2.74 w_bram3_read_addr_1[5]   = i_bram3_read_addr_1[5];   
assign #2.76 w_bram3_read_addr_1[6]   = i_bram3_read_addr_1[6];   
assign #2.73 w_bram3_read_addr_1[7]   = i_bram3_read_addr_1[7];   
assign #2.71 w_bram3_read_addr_1[8]   = i_bram3_read_addr_1[8];   
assign #2.13 w_bram3_ratio_1[0]       = i_bram3_ratio_1[0];       
assign #2.13 w_bram3_ratio_1[1]       = i_bram3_ratio_1[1];       
assign #5.97 w_bram3_pd_1             = i_bram2_3_pd_1[1];        
assign #0.00 w_bram_north_write_clk = i_bram_north_write_clk;
assign #0.00 w_bram_north_read_clk = i_bram_north_read_clk;
assign #0.00 w_bram_south_write_clk = i_bram_south_write_clk;
assign #0.00 w_bram_south_read_clk = i_bram_south_read_clk;
assign #0.00 w_sleep_mode = i_sleep_mode;
assign #3.47 w_bram0_web_0 = i_bram0_web_0;
assign #3.12 w_bram1_web_0 = i_bram1_web_0;
assign #3.07 w_bram2_web_0 = i_bram2_web_0;
assign #2.85 w_bram3_web_0 = i_bram3_web_0; 
assign #2.82 w_bram0_web_1 = i_bram0_web_1;
assign #2.72 w_bram1_web_1 = i_bram1_web_1;
assign #3.07 w_bram2_web_1 = i_bram2_web_1;
assign #2.59 w_bram3_web_1 = i_bram3_web_1; 
north_hbram_top north_hbram (  
    .i_vdd_to_bram0               (w_vdd_to_bram_0_post       ),         
    .i_vdd_to_bram1               (w_vdd_to_bram_0_post       ),
    .i_vdd_to_bram2               (w_vdd_to_bram_0_post       ),
    .i_vdd_to_bram3               (w_vdd_to_bram_0_post       ),
    .i_bram_write_clk             (w_bram_north_write_clk   ),          
    .i_bram_read_clk              (w_bram_north_read_clk    ),          
    .i_bram0_web                  (w_bram0_web_0              ),          
    .i_bram0_wclken               (w_bram0_wclken_0           ),          
    .i_bram0_wclk_inv             (w_bram0_wclk_inv_0         ),          
    .i_bram0_write_addr           (w_bram0_write_addr_0       ),          
    .i_bram0_data_in              (w_bram0_data_in_0          ),          
    .i_bram1_web                  (w_bram1_web_0              ),          
    .i_bram1_wclken               (w_bram1_wclken_0           ),          
    .i_bram1_wclk_inv             (w_bram1_wclk_inv_0         ),          
    .i_bram1_write_addr           (w_bram1_write_addr_0       ),          
    .i_bram1_data_in              (w_bram1_data_in_0          ),          
    .i_bram2_web                  (w_bram2_web_0              ),          
    .i_bram2_wclken               (w_bram2_wclken_0           ),          
    .i_bram2_wclk_inv             (w_bram2_wclk_inv_0         ),          
    .i_bram2_write_addr           (w_bram2_write_addr_0       ),          
    .i_bram2_data_in              (w_bram2_data_in_0          ),          
    .i_bram3_web                  (w_bram3_web_0              ),          
    .i_bram3_wclken               (w_bram3_wclken_0           ),          
    .i_bram3_wclk_inv             (w_bram3_wclk_inv_0         ),          
    .i_bram3_write_addr           (w_bram3_write_addr_0       ),          
    .i_bram3_data_in              (w_bram3_data_in_0          ),          
    .i_bram0_reb                  (w_bram0_reb_0              ),          
    .i_bram0_rclken               (w_bram0_rclken_0           ),          
    .i_bram0_rclk_inv             (w_bram0_rclk_inv_0         ),          
    .i_bram0_read_addr            (w_bram0_read_addr_0        ),          
    .i_bram1_reb                  (w_bram1_reb_0              ),          
    .i_bram1_rclken               (w_bram1_rclken_0           ),          
    .i_bram1_rclk_inv             (w_bram1_rclk_inv_0         ),          
    .i_bram1_read_addr            (w_bram1_read_addr_0        ),          
    .i_bram2_reb                  (w_bram2_reb_0              ),          
    .i_bram2_rclken               (w_bram2_rclken_0           ),          
    .i_bram2_rclk_inv             (w_bram2_rclk_inv_0         ),          
    .i_bram2_read_addr            (w_bram2_read_addr_0        ),          
    .i_bram3_reb                  (w_bram3_reb_0              ),          
    .i_bram3_rclken               (w_bram3_rclken_0           ),          
    .i_bram3_rclk_inv             (w_bram3_rclk_inv_0         ),          
    .i_bram3_read_addr            (w_bram3_read_addr_0        ),          
    .i_bram0_ratio                (w_bram0_ratio_0            ),          
    .i_bram0_pd                   (w_bram0_pd_0               ),          
    .i_bram1_ratio                (w_bram1_ratio_0            ),          
    .i_bram1_pd                   (w_bram1_pd_0               ),          
    .i_bram2_ratio                (w_bram2_ratio_0            ),          
    .i_bram2_pd                   (w_bram2_pd_0               ),          
    .i_bram3_ratio                (w_bram3_ratio_0            ),          
    .i_bram3_pd                   (w_bram3_pd_0               ),          
    .o_bram0_data_out             (o_bram0_data_out_0         ),          
    .o_bram1_data_out             (o_bram1_data_out_0         ),          
    .o_bram2_data_out             (o_bram2_data_out_0         ),          
    .o_bram3_data_out             (o_bram3_data_out_0         ),          
    .i_flag_reset                 (r_reg_BRAM_reset           ),
    .i_config_data                (w_config_data_bram0        ),
    .i_sleep_mode                 (w_sleep_mode               ),
    .i_config_done                (w_config_and_keep0              )
);
south_hbram_top south_hbram (                            
    .i_vdd_to_bram4               (w_vdd_to_bram_1_post       ),         
    .i_vdd_to_bram5               (w_vdd_to_bram_1_post       ),
    .i_vdd_to_bram6               (w_vdd_to_bram_1_post       ),
    .i_vdd_to_bram7               (w_vdd_to_bram_1_post       ),
    .i_bram_write_clk             (w_bram_south_write_clk   ),          
    .i_bram_read_clk              (w_bram_south_read_clk    ),          
    .i_bram4_web                  (w_bram0_web_1              ),          
    .i_bram4_wclken               (w_bram0_wclken_1           ),          
    .i_bram4_wclk_inv             (w_bram0_wclk_inv_1         ),          
    .i_bram4_write_addr           (w_bram0_write_addr_1       ),          
    .i_bram4_data_in              (w_bram0_data_in_1          ),          
    .i_bram5_web                  (w_bram1_web_1              ),          
    .i_bram5_wclken               (w_bram1_wclken_1           ),          
    .i_bram5_wclk_inv             (w_bram1_wclk_inv_1         ),          
    .i_bram5_write_addr           (w_bram1_write_addr_1       ),          
    .i_bram5_data_in              (w_bram1_data_in_1          ),          
    .i_bram6_web                  (w_bram2_web_1              ),          
    .i_bram6_wclken               (w_bram2_wclken_1           ),          
    .i_bram6_wclk_inv             (w_bram2_wclk_inv_1         ),          
    .i_bram6_write_addr           (w_bram2_write_addr_1       ),          
    .i_bram6_data_in              (w_bram2_data_in_1          ),          
    .i_bram7_web                  (w_bram3_web_1              ),          
    .i_bram7_wclken               (w_bram3_wclken_1           ),          
    .i_bram7_wclk_inv             (w_bram3_wclk_inv_1         ),          
    .i_bram7_write_addr           (w_bram3_write_addr_1       ),          
    .i_bram7_data_in              (w_bram3_data_in_1          ),          
    .i_bram4_reb                  (w_bram0_reb_1              ),          
    .i_bram4_rclken               (w_bram0_rclken_1           ),          
    .i_bram4_rclk_inv             (w_bram0_rclk_inv_1         ),          
    .i_bram4_read_addr            (w_bram0_read_addr_1        ),          
    .i_bram5_reb                  (w_bram1_reb_1              ),          
    .i_bram5_rclken               (w_bram1_rclken_1           ),          
    .i_bram5_rclk_inv             (w_bram1_rclk_inv_1         ),          
    .i_bram5_read_addr            (w_bram1_read_addr_1        ),          
    .i_bram6_reb                  (w_bram2_reb_1              ),          
    .i_bram6_rclken               (w_bram2_rclken_1           ),          
    .i_bram6_rclk_inv             (w_bram2_rclk_inv_1         ),          
    .i_bram6_read_addr            (w_bram2_read_addr_1        ),          
    .i_bram7_reb                  (w_bram3_reb_1              ),          
    .i_bram7_rclken               (w_bram3_rclken_1           ),          
    .i_bram7_rclk_inv             (w_bram3_rclk_inv_1         ),          
    .i_bram7_read_addr            (w_bram3_read_addr_1        ),          
    .i_bram4_ratio                (w_bram0_ratio_1            ),          
    .i_bram4_pd                   (w_bram0_pd_1               ),          
    .i_bram5_ratio                (w_bram1_ratio_1            ),          
    .i_bram5_pd                   (w_bram1_pd_1               ),          
    .i_bram6_ratio                (w_bram2_ratio_1            ),          
    .i_bram6_pd                   (w_bram2_pd_1               ),          
    .i_bram7_ratio                (w_bram3_ratio_1            ),          
    .i_bram7_pd                   (w_bram3_pd_1               ),          
    .o_bram4_data_out             (o_bram0_data_out_1         ),          
    .o_bram5_data_out             (o_bram1_data_out_1         ),          
    .o_bram6_data_out             (o_bram2_data_out_1         ),          
    .o_bram7_data_out             (o_bram3_data_out_1         ),           
    .i_flag_reset                 (r_reg_BRAM_reset           ),
    .i_config_data                (w_config_data_bram1        ),
    .i_sleep_mode                 (w_sleep_mode               ),
    .i_config_done                (w_config_and_keep1              )
);                                                                               
endmodule

/****** bram\dual_port_hbram.v ******/
`timescale 1ns/1ps
module bus_mux #(parameter width = 1)(input i_por, input [width - 1:0]a, input [width - 1:0]b, input sel, output [width - 1:0]out);
    assign  out = i_por ? (!sel ? a : b) : 0;
endmodule

/****** bram\north_hbram_top.v ******/
`timescale 1ns/1ps
module north_hbram_top  (
    input wire          i_vdd_to_bram0,
    input wire          i_vdd_to_bram1,
    input wire          i_vdd_to_bram2,
    input wire          i_vdd_to_bram3,
    input wire          i_bram_write_clk,
    input wire          i_bram_read_clk,
    input wire          i_bram0_web,
    input wire          i_bram0_wclken,
    input wire          i_bram0_wclk_inv,
    input wire  [8:0]   i_bram0_write_addr,
    input wire  [7:0]   i_bram0_data_in,
    input wire          i_bram1_web,
    input wire          i_bram1_wclken,
    input wire          i_bram1_wclk_inv,
    input wire  [8:0]   i_bram1_write_addr,
    input wire  [7:0]   i_bram1_data_in,
    input wire          i_bram2_web,
    input wire          i_bram2_wclken,
    input wire          i_bram2_wclk_inv,
    input wire  [8:0]   i_bram2_write_addr,
    input wire  [7:0]   i_bram2_data_in,
    input wire          i_bram3_web,
    input wire          i_bram3_wclken,
    input wire          i_bram3_wclk_inv,
    input wire  [8:0]   i_bram3_write_addr,
    input wire  [7:0]   i_bram3_data_in,
    input  wire         i_bram0_reb,
    input  wire         i_bram0_rclken,
    input  wire         i_bram0_rclk_inv,
    input  wire [8:0]   i_bram0_read_addr,
    input  wire         i_bram1_reb,
    input  wire         i_bram1_rclken,
    input  wire         i_bram1_rclk_inv,
    input  wire [8:0]   i_bram1_read_addr,
    input  wire         i_bram2_reb,
    input  wire         i_bram2_rclken,
    input  wire         i_bram2_rclk_inv,
    input  wire [8:0]   i_bram2_read_addr,
    input  wire         i_bram3_reb,
    input  wire         i_bram3_rclken,
    input  wire         i_bram3_rclk_inv,
    input  wire [8:0]   i_bram3_read_addr,
    input wire  [1:0]   i_bram0_ratio ,
    input wire          i_bram0_pd    ,
    input wire  [1:0]   i_bram1_ratio ,
    input wire          i_bram1_pd    ,   
    input wire  [1:0]   i_bram2_ratio ,
    input wire          i_bram2_pd    ,   
    input wire  [1:0]   i_bram3_ratio ,
    input wire          i_bram3_pd    ,
    input wire            i_flag_reset,
    input wire [16383:0]  i_config_data,
    input wire            i_sleep_mode,
    input wire            i_config_done,
    output wire [7:0]     o_bram0_data_out, 
    output wire [7:0]     o_bram1_data_out,
    output wire [7:0]     o_bram2_data_out,     
    output wire [7:0]     o_bram3_data_out      
);
wire [4095:0] w_config_data_sram0;
wire [4095:0] w_config_data_sram1;
wire [4095:0] w_config_data_sram2;
wire [4095:0] w_config_data_sram3;
assign w_config_data_sram0 = i_config_data [4095 :0    ];
assign w_config_data_sram1 = i_config_data [8191 :4096 ];
assign w_config_data_sram2 = i_config_data [12287:8192 ];
assign w_config_data_sram3 = i_config_data [16383:12288];
dual_port_bram BRAM_0 (
.i_web            (i_bram0_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram0_wclken       ),
.i_wclk_inv       (i_bram0_wclk_inv     ),
.i_waddr          (i_bram0_write_addr   ),
.i_din            (i_bram0_data_in      ),
.i_reb            (i_bram0_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram0_rclken       ),
.i_rclk_inv       (i_bram0_rclk_inv     ),
.i_raddr          (i_bram0_read_addr    ),
.i_ratio          (i_bram0_ratio        ),
.i_pd             (i_bram0_pd           ),
.i_por            (i_vdd_to_bram0       ),
.o_dout           (o_bram0_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram0  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )       
);
dual_port_bram BRAM_1 (
.i_web            (i_bram1_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram1_wclken       ),
.i_wclk_inv       (i_bram1_wclk_inv     ),
.i_waddr          (i_bram1_write_addr   ),
.i_din            (i_bram1_data_in      ),
.i_reb            (i_bram1_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram1_rclken       ),
.i_rclk_inv       (i_bram1_rclk_inv     ),
.i_raddr          (i_bram1_read_addr    ),
.i_ratio          (i_bram1_ratio        ),
.i_pd             (i_bram1_pd           ),
.i_por            (i_vdd_to_bram1       ),
.o_dout           (o_bram1_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram1  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )        
);
dual_port_bram BRAM_2 (
.i_web            (i_bram2_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram2_wclken       ),
.i_wclk_inv       (i_bram2_wclk_inv     ),
.i_waddr          (i_bram2_write_addr   ),
.i_din            (i_bram2_data_in      ),
.i_reb            (i_bram2_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram2_rclken       ),
.i_rclk_inv       (i_bram2_rclk_inv     ),
.i_raddr          (i_bram2_read_addr    ),
.i_ratio          (i_bram2_ratio        ),
.i_pd             (i_bram2_pd           ),
.i_por            (i_vdd_to_bram2       ),
.o_dout           (o_bram2_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram2  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )         
);
dual_port_bram BRAM_3 (
.i_web            (i_bram3_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram3_wclken       ),
.i_wclk_inv       (i_bram3_wclk_inv     ),
.i_waddr          (i_bram3_write_addr   ),
.i_din            (i_bram3_data_in      ),
.i_reb            (i_bram3_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram3_rclken       ),
.i_rclk_inv       (i_bram3_rclk_inv     ),
.i_raddr          (i_bram3_read_addr    ),
.i_ratio          (i_bram3_ratio        ),
.i_pd             (i_bram3_pd           ),
.i_por            (i_vdd_to_bram3       ),
.o_dout           (o_bram3_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram3  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )        
);
endmodule

/****** bram\south_hbram_top.v ******/
`timescale 1ns/1ps
module south_hbram_top  (
    input wire          i_vdd_to_bram4,
    input wire          i_vdd_to_bram5,
    input wire          i_vdd_to_bram6,
    input wire          i_vdd_to_bram7,
    input wire          i_bram_write_clk,
    input wire          i_bram_read_clk,
    input wire          i_bram4_web,
    input wire          i_bram4_wclken,
    input wire          i_bram4_wclk_inv,
    input wire  [8:0]   i_bram4_write_addr,
    input wire  [7:0]   i_bram4_data_in,
    input wire          i_bram5_web,
    input wire          i_bram5_wclken,
    input wire          i_bram5_wclk_inv,
    input wire  [8:0]   i_bram5_write_addr,
    input wire  [7:0]   i_bram5_data_in,
    input wire          i_bram6_web,
    input wire          i_bram6_wclken,
    input wire          i_bram6_wclk_inv,
    input wire  [8:0]   i_bram6_write_addr,
    input wire  [7:0]   i_bram6_data_in,
    input wire          i_bram7_web,
    input wire          i_bram7_wclken,
    input wire          i_bram7_wclk_inv,
    input wire  [8:0]   i_bram7_write_addr,
    input wire  [7:0]   i_bram7_data_in,
    input wire          i_bram4_reb,
    input wire          i_bram4_rclken,
    input wire          i_bram4_rclk_inv,
    input wire  [8:0]   i_bram4_read_addr,
    input wire          i_bram5_reb,
    input wire          i_bram5_rclken,
    input wire          i_bram5_rclk_inv,
    input wire  [8:0]   i_bram5_read_addr,
    input wire          i_bram6_reb,
    input wire          i_bram6_rclken,
    input wire          i_bram6_rclk_inv,
    input wire  [8:0]   i_bram6_read_addr,
    input wire          i_bram7_reb,
    input wire          i_bram7_rclken,
    input wire          i_bram7_rclk_inv,
    input wire  [8:0]   i_bram7_read_addr,
    input wire  [1:0]   i_bram4_ratio ,
    input wire          i_bram4_pd    ,
    input wire  [1:0]   i_bram5_ratio ,
    input wire          i_bram5_pd    ,   
    input wire  [1:0]   i_bram6_ratio ,
    input wire          i_bram6_pd    ,   
    input wire  [1:0]   i_bram7_ratio ,
    input wire          i_bram7_pd    ,
    input wire          i_flag_reset,
    input wire [16383:0]  i_config_data,
    input wire            i_sleep_mode,
    input wire            i_config_done,
    output wire [7:0]     o_bram4_data_out, 
    output wire [7:0]     o_bram5_data_out,
    output wire [7:0]     o_bram6_data_out,     
    output wire [7:0]     o_bram7_data_out      
);
wire [4095:0] w_config_data_sram0;
wire [4095:0] w_config_data_sram1;
wire [4095:0] w_config_data_sram2;
wire [4095:0] w_config_data_sram3;
assign w_config_data_sram0 = i_config_data [4095 :0    ];
assign w_config_data_sram1 = i_config_data [8191 :4096 ];
assign w_config_data_sram2 = i_config_data [12287:8192 ];
assign w_config_data_sram3 = i_config_data [16383:12288];
dual_port_bram BRAM_4 (
.i_web            (i_bram4_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram4_wclken       ),
.i_wclk_inv       (i_bram4_wclk_inv     ),
.i_waddr          (i_bram4_write_addr   ),
.i_din            (i_bram4_data_in      ),
.i_reb            (i_bram4_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram4_rclken       ),
.i_rclk_inv       (i_bram4_rclk_inv     ),
.i_raddr          (i_bram4_read_addr    ),
.i_ratio          (i_bram4_ratio        ),
.i_pd             (i_bram4_pd           ),
.i_por            (i_vdd_to_bram4       ),
.o_dout           (o_bram4_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram0  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )  
);
dual_port_bram BRAM_5 (
.i_web            (i_bram5_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram5_wclken       ),
.i_wclk_inv       (i_bram5_wclk_inv     ),
.i_waddr          (i_bram5_write_addr   ),
.i_din            (i_bram5_data_in      ),
.i_reb            (i_bram5_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram5_rclken       ),
.i_rclk_inv       (i_bram5_rclk_inv     ),
.i_raddr          (i_bram5_read_addr    ),
.i_ratio          (i_bram5_ratio        ),
.i_pd             (i_bram5_pd           ),
.i_por            (i_vdd_to_bram5       ),
.o_dout           (o_bram5_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram1  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )             
);
dual_port_bram BRAM_6 (
.i_web            (i_bram6_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram6_wclken       ),
.i_wclk_inv       (i_bram6_wclk_inv     ),
.i_waddr          (i_bram6_write_addr   ),
.i_din            (i_bram6_data_in      ),
.i_reb            (i_bram6_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram6_rclken       ),
.i_rclk_inv       (i_bram6_rclk_inv     ),
.i_raddr          (i_bram6_read_addr    ),
.i_ratio          (i_bram6_ratio        ),
.i_pd             (i_bram6_pd           ),
.i_por            (i_vdd_to_bram6       ),
.o_dout           (o_bram6_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram2  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )             
);
dual_port_bram BRAM_7 (
.i_web            (i_bram7_web          ),
.i_wclk           (i_bram_write_clk     ),
.i_wclken         (i_bram7_wclken       ),
.i_wclk_inv       (i_bram7_wclk_inv     ),
.i_waddr          (i_bram7_write_addr   ),
.i_din            (i_bram7_data_in      ),
.i_reb            (i_bram7_reb          ),
.i_rclk           (i_bram_read_clk      ),
.i_rclken         (i_bram7_rclken       ),
.i_rclk_inv       (i_bram7_rclk_inv     ),
.i_raddr          (i_bram7_read_addr    ),
.i_ratio          (i_bram7_ratio        ),
.i_pd             (i_bram7_pd           ),
.i_por            (i_vdd_to_bram7       ),
.o_dout           (o_bram7_data_out     ),
.i_flag_reset     (i_flag_reset         ),
.i_config_data    (w_config_data_sram3  ),
.i_sleep_mode     (i_sleep_mode         ),
.i_config_done    (i_config_done        )              
);
endmodule

/****** clk_ctrl\clk_ctrl.v ******/

`timescale 1ns/1ps
module clk_ctrl 
(
    input   wire        i_por_raw            ,
    input   wire        i_osc_en_request     ,
    input   wire        i_iob_osc_en         ,
    input   wire        i_func_mode          ,
    input   wire        i_reg_osc_en_2ff_sync,
    input   wire        i_all_user_clk_nrst  ,
    input   wire        i_pll_en_request     ,
    input   wire        i_iob_pll_en         ,
    input   wire        i_pll_ref_clk_sel    ,
    input   wire        i_ref_clk_ext        ,
    input   wire [5:0]  i_reg_refdiv         ,
    input   wire [11:0] i_reg_fbdiv          ,
    input   wire [2:0]  i_reg_posdiv1_out    ,
    input   wire [2:0]  i_reg_posdiv2_out    ,
    input   wire        i_reg_bypass         ,
    input   wire        i_en_user_clk_sync   ,
    input   wire        i_pll_en_override    ,
    output  wire        o_osc_ready_user     ,
    output  wire        o_osc_ready_block    ,
    output  wire        o_soc_osc_clk        ,
    output  wire        o_user_osc_clk       ,
    output  wire        o_pll_lock           ,         
    output  wire        o_pll_lock_block     ,   
    output  wire        o_pll_soc_fout       ,     
    output  wire        o_pll_user_fout    
);
osc_ctrl osc_ctrl 
(
    .i_por_raw             (i_por_raw            ),
    .i_osc_en_request      (i_osc_en_request     ),
    .i_iob_osc_en          (i_iob_osc_en         ),
    .i_func_mode           (i_func_mode          ),
    .i_reg_osc_en_2ff_sync (i_reg_osc_en_2ff_sync),
    .i_all_user_clk_nrst   (i_all_user_clk_nrst  ),
    .o_osc_ready_user      (o_osc_ready_user     ),
    .o_osc_ready_block     (o_osc_ready_block    ),
    .o_soc_osc_clk         (o_soc_osc_clk        ),
    .o_user_osc_clk        (o_user_osc_clk       )
);
    wire w_osc_clk_block;
    assign w_osc_clk_block = osc_ctrl.w_osc_fout;
pll_ctrl pll_ctrl (
    .i_por_raw                 (i_por_raw           ),        
    .i_pll_en_override         (i_pll_en_override   ),
    .i_iob_pll_en              (i_iob_pll_en        ),
    .i_func_mode               (i_func_mode         ),
    .i_reg_pll_en_2ff_sync     (i_en_user_clk_sync  ),
    .i_all_user_clk_nrst       (i_all_user_clk_nrst ),
    .i_pll_ref_clk_sel         (i_pll_ref_clk_sel   ),
    .i_ref_clk_ext             (i_ref_clk_ext       ),
    .i_ref_clk_osc             (w_osc_clk_block     ),
    .i_reg_refdiv              (i_reg_refdiv        ),
    .i_reg_fbdiv               (i_reg_fbdiv         ),
    .i_reg_posdiv1_out         (i_reg_posdiv1_out   ),
    .i_reg_posdiv2_out         (i_reg_posdiv2_out   ),
    .i_iob_bypass              (i_reg_bypass        ),
    .o_pll_lock_user           (o_pll_lock          ),
    .pll_lock_block            (o_pll_lock_block    ),
    .o_soc_pll_clk             (o_pll_soc_fout      ),
    .o_user_pll_clk            (o_pll_user_fout     )
);
endmodule

/****** ctrl\ctrl.v ******/
`timescale 1ns/1ns
module ctrl #(
    parameter BG_FORCE_DLY_CNT          = 500,  
    parameter PRE_LOAD_BITSTREAM_CNT    = 500, 
    parameter LOAD_BITSTREAM_CNT        = 5000, 
    parameter LOAD_INIT_CNT             = 1000 
) (
    input wire i_vdd_ok,
    input wire i_por,
    input wire i_nRST,
    input wire i_nSLEEP,
    input wire i_ctrl_clk,
    input wire i_ctrl_clk_rdy,
    input wire i_user_clk_ready,
    input wire i_all_user_clk_low,
    input wire i_force_dev_sleep_dis,
    input wire i_gpio_keep,
    output wire o_retention_cfg_reg_set,
    output wire o_retention_iob_set,
    output wire o_retention_cfg_reg_rst,
    output wire o_retention_iob_rst,
    output wire o_ctrl_gpio_fn_inp_gating,
    output wire o_ctrl_gpio_fn_out_gating,
    output wire o_ctrl_gpio_fn_oe_gating,
    output wire o_ctrl_gpio_1x_pullup_en_reg ,
    output wire o_ctrl_gpio_2x_pullup_en_reg ,
    output wire o_ctrl_gpio_2x_buffer_en_reg ,
    output wire o_ctrl_gpio_open_drain_en_reg,
    output wire o_nSLEEP_nRST_pullup_en,
    output wire o_ctrl_clk_req,
    output wire [4:0] o_ctrl_current_state,
    output wire o_pll_override,
    output wire o_osc_en_override,
    output wire o_iob_func_mode,
    output wire o_all_user_clk_rst,
    output wire o_nRST_iso_en,
    output wire o_bram_sleep_en,
    output wire o_bram_rst_en,
    output wire o_config_done
);
    reg r_retention_cfg_reg_set;  
    reg r_retention_iob_set;
    reg r_retention_cfg_reg_rst;  
    reg r_retention_iob_rst;
    reg r_ctrl_gpio_fn_inp_gating;
    reg r_ctrl_gpio_fn_out_gating;
    reg r_ctrl_gpio_fn_oe_gating; 
    reg r_ctrl_gpio_1x_pullup_en_reg ;
    reg r_ctrl_gpio_2x_pullup_en_reg ;
    reg r_ctrl_gpio_2x_buffer_en_reg ;
    reg r_ctrl_gpio_open_drain_en_reg;
    assign o_ctrl_gpio_1x_pullup_en_reg  = r_ctrl_gpio_1x_pullup_en_reg ;
    assign o_ctrl_gpio_2x_pullup_en_reg  = r_ctrl_gpio_2x_pullup_en_reg ;
    assign o_ctrl_gpio_2x_buffer_en_reg  = r_ctrl_gpio_2x_buffer_en_reg ;
    assign o_ctrl_gpio_open_drain_en_reg = r_ctrl_gpio_open_drain_en_reg;
    assign o_retention_cfg_reg_set = r_retention_cfg_reg_set;   
    assign o_retention_iob_set = r_retention_iob_set;
    assign o_retention_cfg_reg_rst = r_retention_cfg_reg_rst;   
    assign o_retention_iob_rst = r_retention_iob_rst;
    assign o_ctrl_gpio_fn_inp_gating = r_ctrl_gpio_fn_inp_gating;       
    assign o_ctrl_gpio_fn_out_gating = r_ctrl_gpio_fn_out_gating;       
    assign o_ctrl_gpio_fn_oe_gating = r_ctrl_gpio_fn_oe_gating;       
    reg r_nSLEEP_nRST_pullup_en;
    assign o_nSLEEP_nRST_pullup_en = r_nSLEEP_nRST_pullup_en;
    reg r_ctrl_clk_req;
    assign o_ctrl_clk_req = r_ctrl_clk_req;
    reg r_pll_override;
    assign o_pll_override = r_pll_override;
    reg r_all_user_clk_rst;
    assign o_all_user_clk_rst = r_all_user_clk_rst;
    reg r_iob_func_mode;
    assign o_iob_func_mode = r_iob_func_mode;
    reg r_nRST_iso_en;
    assign o_nRST_iso_en = r_nRST_iso_en;
    reg r_bram_sleep_en;
    assign o_bram_sleep_en = r_bram_sleep_en;
    reg r_bram_rst_en;
    assign o_bram_rst_en = r_bram_rst_en;
    reg r_config_done;
    assign o_config_done = r_config_done;
    localparam [4:0] S_IDLE                     = `S_IDLE;
    localparam [4:0] S_PWR_OFF                  = `S_PWR_OFF;
    localparam [4:0] S_FORCE_BG                 = `S_FORCE_BG;
    localparam [4:0] S_PRE_LOAD                 = `S_PRE_LOAD;
    localparam [4:0] S_LOAD_BITSTREAM           = `S_LOAD_BITSTREAM;
    localparam [4:0] S_LOAD_INIT                = `S_LOAD_INIT;
    localparam [4:0] S_FUNC_MODE                = `S_FUNC_MODE;
    localparam [4:0] S_RETENTION_SEQ_START      = `S_RETENTION_SEQ_START;
    localparam [4:0] S_RST_MODE                 = `S_RST_MODE;
    localparam [4:0] S_SLEEP_MODE               = `S_SLEEP_MODE;
    localparam GPIO_INIT_CNT = LOAD_INIT_CNT/3;
    localparam RESTART_AFTER_RST_EN = 0;
    reg [4:0] r_ctrl_current_state, r_ctrl_next_state;
    wire i_ctrl_clk_icg;
    assign i_ctrl_clk_icg = i_ctrl_clk & i_ctrl_clk_rdy;
    reg r_event_ret_seq_start;
    reg r_nrst_mode;
    initial begin
        r_ctrl_current_state = S_PWR_OFF;
        r_ret_seq_dly_cnt1 = 0;
        r_ret_seq_dly_cnt2 = 0;
        r_bg_force_dly_cnt = 0;
        r_load_bitstream_cnt = 0;
        r_load_init_cnt = 0;
    end
    always @(posedge i_ctrl_clk_icg, negedge i_por, negedge i_nRST) begin
        if (!i_por) begin
            r_ctrl_current_state <= S_PWR_OFF;
        end
        else if (!i_nRST & !r_config_done) begin
            r_ctrl_current_state <= S_IDLE;
        end
        else begin
            r_ctrl_current_state <= r_ctrl_next_state;
        end
    end
    assign o_ctrl_current_state = r_ctrl_current_state;
    always @(*) begin
        case (r_ctrl_current_state)
            S_PWR_OFF: begin
                if (i_por) begin
                    if (i_nRST) begin
                        r_ctrl_next_state = S_IDLE;
                        r_ctrl_clk_req = 1'b1;
                        r_pll_override = 1'b1;
                    end
                end
                else begin
                    r_ctrl_next_state = S_PWR_OFF;
                    r_ctrl_clk_req = 1'b0;
                    r_retention_cfg_reg_set = 1'b0;  
                    r_retention_iob_set = 1'b0;
                    r_retention_cfg_reg_rst = 1'b0;  
                    r_retention_iob_rst = 1'b0;
                    r_ctrl_gpio_fn_inp_gating = 1'b0;
                    r_ctrl_gpio_fn_out_gating = 1'b0;
                    r_ctrl_gpio_fn_oe_gating = 1'b0;
                    r_nSLEEP_nRST_pullup_en = 1'b0;
                    r_ctrl_gpio_1x_pullup_en_reg  = 1'b0;
                    r_ctrl_gpio_2x_pullup_en_reg  = 1'b0;
                    r_ctrl_gpio_2x_buffer_en_reg  = 1'b0;
                    r_ctrl_gpio_open_drain_en_reg = 1'b0;
                    r_pll_override = 0;
                    r_config_done = 0;
                    r_event_ret_seq_start = 0;
                    r_iob_func_mode   = 1'b0;
                    r_all_user_clk_rst = 1'b1;
                    r_bg_ok = 0;
                    r_nSLEEP_low_detected = 0;
                    r_nrst_mode = 0;
                    r_bram_sleep_en = 0;
                    r_bram_rst_en = 0;
                    if (i_vdd_ok) begin
                        r_nRST_iso_en = 1;
                        r_nSLEEP_nRST_pullup_en = 1'b1;
                    end
                    else begin
                        r_nRST_iso_en = 0;
                        r_nSLEEP_nRST_pullup_en = 1'b0;
                    end
                end
            end
            S_IDLE: begin
                r_config_done = 0;
                r_all_user_clk_rst = 1'b1;
                r_bg_ok = 0;
                r_bram_sleep_en = 0;
                if (i_nRST) begin
                    r_ctrl_clk_req = 1'b1;
                    r_pll_override = 1'b1;
                    if (i_ctrl_clk_rdy) begin
                        r_ctrl_next_state = S_FORCE_BG;
                    end
                end
                else begin
                    r_ctrl_clk_req = 1'b0;
                    r_pll_override = 1'b0;
                end
            end
            S_FORCE_BG: begin
                if (r_bg_force_dly_cnt == BG_FORCE_DLY_CNT) begin
                    r_bg_ok = 1;
                    r_ctrl_next_state = S_PRE_LOAD;
                end
                else begin
                    r_nSLEEP_nRST_pullup_en = 1'b0;
                    r_ctrl_next_state = S_FORCE_BG;
                end
            end
            S_PRE_LOAD: begin
                if (r_pre_load_bitstream_cnt == PRE_LOAD_BITSTREAM_CNT) begin
                    r_ctrl_next_state = S_LOAD_BITSTREAM;
                    r_nRST_iso_en = 0;
                end
                else begin
                    r_ctrl_next_state = S_PRE_LOAD;
                end
            end
            S_LOAD_BITSTREAM: begin
                if (r_load_bitstream_cnt == LOAD_BITSTREAM_CNT) begin
                    r_config_done = 1;
                    r_all_user_clk_rst = 1'b0;
                    r_ctrl_next_state = S_LOAD_INIT;
                end
                else begin
                    r_bram_rst_en = 0;
                    r_config_done = 0;
                    r_all_user_clk_rst = 1'b1;
                    r_ctrl_next_state = S_LOAD_BITSTREAM;
                end
            end
            S_LOAD_INIT: begin
                if (r_load_init_cnt == LOAD_INIT_CNT-2) begin
                    r_pll_override = 1'b0;
                end
                if (r_load_init_cnt == LOAD_INIT_CNT) begin
                    if (i_user_clk_ready) begin
                        r_ctrl_next_state = S_FUNC_MODE;
                        r_retention_cfg_reg_rst = 1'b0;
                        r_retention_iob_rst = 1'b0;
                        r_iob_func_mode   = 1'b1;
                    end
                end
                else begin
                    r_retention_cfg_reg_rst = 1'b1;
                    r_ctrl_gpio_fn_inp_gating = 1'b1;
                    r_ctrl_gpio_1x_pullup_en_reg  = 1'b1;
                    r_ctrl_gpio_2x_pullup_en_reg  = 1'b1;
                    r_ctrl_gpio_2x_buffer_en_reg  = 1'b1;
                    r_ctrl_gpio_open_drain_en_reg = 1'b1;
                    r_bram_sleep_en = 0;  
                    if (r_load_init_cnt == GPIO_INIT_CNT) begin 
                        r_ctrl_gpio_fn_out_gating = 1'b1;
                        r_ctrl_gpio_fn_oe_gating = 1'b1;
                        r_retention_iob_rst = 1'b1;
                    end 
                    r_ctrl_next_state = S_LOAD_INIT;
                end
                r_event_ret_seq_start = 0;
            end
            S_FUNC_MODE: begin
                if ((!i_nRST || !i_nSLEEP) && (!r_event_ret_seq_start)) begin
                    r_event_ret_seq_start = 1;
                    r_iob_func_mode = 0;
                    r_ctrl_clk_req = 1;
                    r_ctrl_next_state = S_RETENTION_SEQ_START;
                    if (!i_nRST) begin
                        r_nrst_mode = 1;
                    end
                    else begin
                        r_nrst_mode = 0;
                    end
                end
                else begin
                    if (!r_event_ret_seq_start) begin 
                        r_ctrl_clk_req    = 1'b0;
                        r_ctrl_next_state = S_FUNC_MODE;
                    end
                end
            end
            S_RETENTION_SEQ_START: begin
                r_nSLEEP_low_detected = 0;
                if (i_all_user_clk_low) begin
                    r_pll_override = 1;
                    if (r_ret_seq_dly_cnt1 == 10) begin
                        if (r_nrst_mode) begin
                            if (i_gpio_keep) begin
                                r_retention_cfg_reg_set = 1'b1;
                                r_retention_iob_set = 1'b1;
                                r_ctrl_gpio_1x_pullup_en_reg  = 1;
                                r_ctrl_gpio_2x_pullup_en_reg  = 1;
                                r_ctrl_gpio_2x_buffer_en_reg  = 1;
                                r_ctrl_gpio_open_drain_en_reg = 1;
                            end
                            else begin
                                r_retention_cfg_reg_set = 1'b0;
                                r_retention_iob_set = 1'b0;
                                r_ctrl_gpio_1x_pullup_en_reg  = 0;
                                r_ctrl_gpio_2x_pullup_en_reg  = 0;
                                r_ctrl_gpio_2x_buffer_en_reg  = 0;
                                r_ctrl_gpio_open_drain_en_reg = 0;
                            end
                            r_bram_sleep_en = 0;
                            r_bram_rst_en = 1;
                            r_ctrl_next_state = S_RST_MODE;
                        end
                        else begin
                            r_bram_sleep_en = 1;
                            r_bram_rst_en = 0;
                            r_retention_cfg_reg_set = 1'b1;
                            r_retention_iob_set = 1'b1;
                            r_ctrl_next_state = S_SLEEP_MODE;
                        end
                    end
                end
                else begin
                    if (!i_force_dev_sleep_dis) begin
                        if (r_ret_seq_dly_cnt2 == 250) begin
                            r_all_user_clk_rst = 1;
                        end
                    end
                end
            end
            S_SLEEP_MODE: begin
                r_retention_cfg_reg_set = 1'b0;
                r_retention_iob_set = 1'b0;
                r_ctrl_gpio_fn_out_gating = 0;
                r_ctrl_gpio_fn_oe_gating = 0;
                r_ctrl_clk_req    = 1'b0;
                r_all_user_clk_rst = 1'b0;
                if (!i_nSLEEP || r_nSLEEP_low_detected) begin
                    r_nSLEEP_low_detected = 1;
                    if (i_nSLEEP) begin
                        r_ctrl_clk_req = 1;
                        r_ctrl_next_state = S_LOAD_INIT;
                    end
                end
                else if(!r_nSLEEP_low_detected) begin
                    r_ctrl_clk_req = 1;
                    r_ctrl_next_state = S_SLEEP_MODE;
                end
            end
            S_RST_MODE: begin
                r_ctrl_gpio_fn_inp_gating = 1'b0;
                r_ctrl_gpio_fn_out_gating = 0;
                r_ctrl_gpio_fn_oe_gating = 0;
                r_retention_cfg_reg_set = 1'b0;
                r_retention_iob_set = 1'b0;
                r_ctrl_gpio_1x_pullup_en_reg  = 0;
                r_ctrl_gpio_2x_pullup_en_reg  = 0;
                r_ctrl_gpio_2x_buffer_en_reg  = 0;
                r_ctrl_gpio_open_drain_en_reg = 0;
                if (RESTART_AFTER_RST_EN) begin
                    r_ctrl_next_state = S_IDLE;
                end
                else begin
                    r_config_done = 0;
                    r_all_user_clk_rst = 1'b1;
                    r_bg_ok = 0;
                    r_bram_sleep_en = 0;
                    r_ctrl_clk_req = 1'b0;
                    r_pll_override = 1'b0;
                end
            end
            default: ;
        endcase;
    end
reg r_nSLEEP_low_detected;
reg r_bg_ok;
    reg [15:0] r_ret_seq_dly_cnt1 ;
    always @(posedge i_ctrl_clk_icg, negedge i_por) begin :  CNT_RET_SEQ_DLY1
        if (!i_por) begin
            r_ret_seq_dly_cnt1 <= 0;
        end
        else if ((r_ctrl_current_state == S_RETENTION_SEQ_START) && (r_ret_seq_dly_cnt1 != 10) && (i_all_user_clk_low)) begin
            r_ret_seq_dly_cnt1 <= r_ret_seq_dly_cnt1 + 1;
        end
        else if ((r_ctrl_current_state == S_RETENTION_SEQ_START) && (i_all_user_clk_low)) begin
            r_ret_seq_dly_cnt1 <= 0;
        end
    end
    reg [15:0] r_ret_seq_dly_cnt2 ;
    always @(posedge i_ctrl_clk_icg, negedge i_por) begin :  CNT_RET_SEQ_DLY2
        if (!i_por) begin
            r_ret_seq_dly_cnt2 <= 0;
        end
        else if ((r_ctrl_current_state == S_RETENTION_SEQ_START) && (r_ret_seq_dly_cnt2 != 250) && (!i_all_user_clk_low) && (!i_force_dev_sleep_dis)) begin
            r_ret_seq_dly_cnt2 <= r_ret_seq_dly_cnt2 + 1;
        end
        else if ((r_ctrl_current_state == S_RETENTION_SEQ_START) && (i_all_user_clk_low)) begin
            r_ret_seq_dly_cnt2 <= 0;
        end
    end
    reg [15:0] r_bg_force_dly_cnt ;
    always @(posedge i_ctrl_clk_icg, negedge i_por) begin : CNT_BG_FORCE_DLY 
        if (!i_por) begin
            r_bg_force_dly_cnt <= 0;
        end
        else if ((r_ctrl_current_state == S_FORCE_BG) && (r_bg_force_dly_cnt !=BG_FORCE_DLY_CNT)) begin
            r_bg_force_dly_cnt <= r_bg_force_dly_cnt + 1;
        end
        else if (r_ctrl_current_state == S_FORCE_BG) begin
            r_bg_force_dly_cnt <= 0;
        end
    end
    reg [15:0] r_load_bitstream_cnt;
    always @(posedge i_ctrl_clk_icg, negedge i_por) begin : CNT_LOAD_BITSTREAM 
        if (!i_por) begin
            r_load_bitstream_cnt <= 0;
        end
        else if ((r_ctrl_current_state == S_LOAD_BITSTREAM) && (r_load_bitstream_cnt != LOAD_BITSTREAM_CNT)) begin
            r_load_bitstream_cnt <= r_load_bitstream_cnt + 1;
        end
        else if ((r_ctrl_current_state == S_LOAD_BITSTREAM)) begin
            r_load_bitstream_cnt <= 0;
        end
    end
    reg [15:0] r_pre_load_bitstream_cnt;
    always @(posedge i_ctrl_clk_icg, negedge i_por) begin : PRE_CNT_LOAD_BITSTREAM 
        if (!i_por) begin
            r_pre_load_bitstream_cnt <= 0;
        end
        else if ((r_ctrl_current_state == S_PRE_LOAD) && (r_pre_load_bitstream_cnt != PRE_LOAD_BITSTREAM_CNT)) begin
            r_pre_load_bitstream_cnt <= r_pre_load_bitstream_cnt + 1;
        end
        else if ((r_ctrl_current_state == S_PRE_LOAD)) begin
            r_pre_load_bitstream_cnt <= 0;
        end
    end
    reg [15:0] r_load_init_cnt ;
    always @(posedge i_ctrl_clk_icg, negedge i_por) begin : CNT_LOAD_INIT 
        if (!i_por) begin
            r_load_init_cnt <= 0;
        end
        else if ((r_ctrl_current_state == S_LOAD_INIT) && (r_load_init_cnt == LOAD_INIT_CNT) && (!i_user_clk_ready)) begin
            r_load_init_cnt <= r_load_init_cnt;
        end
        else if ((r_ctrl_current_state == S_LOAD_INIT) && (r_load_init_cnt != LOAD_INIT_CNT)) begin
            r_load_init_cnt <= r_load_init_cnt + 1;
        end
        else if ((r_ctrl_current_state == S_LOAD_INIT)) begin
            r_load_init_cnt <= 0;
        end
    end
endmodule

/****** gpio\gpio.v ******/
`timescale 1ns/100ps
module gpio 
(
    input wire       i_vddc,
    input wire       i_vddio,
    input wire       i_1x_pullup_en,
    input wire       i_2x_pullup_en,
    output wire      o_digital_input,
    input wire       i_open_drain_en,
    input wire       i_2x_buffer_en,
    input wire       i_digital_out_en,
    input wire       i_digital_output,
    inout wire       pad
);
    wire w_vdd_ok;
    assign w_vdd_ok = i_vddc & i_vddio;
        assign o_digital_input      = w_vdd_ok ? &pad : 1'bx;
        wire w_1x_output_buf_enb, w_2x_output_buf_enb, w_output_buf_enb;
        assign w_1x_output_buf_enb  = ~i_digital_out_en | (i_digital_output & i_open_drain_en);
        assign w_2x_output_buf_enb  = w_1x_output_buf_enb | ~i_2x_buffer_en;
        assign w_output_buf_enb     = w_vdd_ok ? w_1x_output_buf_enb & w_2x_output_buf_enb : 1'b1;
        bufif0 output_buf (pad, i_digital_output, w_output_buf_enb);
        assign (pull1, highz0) pad = (i_1x_pullup_en || i_2x_pullup_en) && w_vdd_ok ? 1'b1 : 1'bz;
endmodule

/****** logic_as_clk\logic_as_clk.v ******/

`timescale 1ns/1ns
module logic_as_clock 
(
    input  wire  i_all_user_clk_nrst,           
    input  wire  i_func_mode,                   
    input  wire  i_iob_data_as_clk_en_user,         
    input  wire  i_iob_data_as_clk_user,            
    input  wire  i_reg_data_as_clk_en_2ff_sync,
    output wire  o_data_as_clk_in_user,             
    output wire  o_data_as_clk_ready_soc
);
    reg      r_data_as_clk_ready_soc;    
    wire     w_not_iob_data_as_clk_user;
    wire     w_func_mode_sync;
    wire     w_iob_data_as_clk_en_user_sync;
    wire     w_iob_data_as_clk_en_user_muxed;
    wire     w_clk_internal;
    wire     w_icg_en_internal;
    wire     w_data_as_clk_in_user;      
    not not_i_iob_data_as_clk_user ( w_not_iob_data_as_clk_user,
                                     i_iob_data_as_clk_user
                                   );
    i2xDFF u_block_2xDFF_func_mode_soc_sync ( .i_clk     (w_not_iob_data_as_clk_user),
                                              .i_resetb  (i_all_user_clk_nrst       ),
                                              .i_set     (1'b0                      ),
                                              .i_data    (i_func_mode               ),
                                              .o_qout    (w_func_mode_sync          )
                                            );
    i2xDFF u_block_2xDFF_data_as_clk_en_sync ( .i_clk     (i_iob_data_as_clk_user        ),
                                               .i_resetb  (i_all_user_clk_nrst           ),
                                               .i_set     (1'b0                          ),
                                               .i_data    (i_iob_data_as_clk_en_user     ),
                                               .o_qout    (w_iob_data_as_clk_en_user_sync) 
                                             );
    assign w_iob_data_as_clk_en_user_muxed = i_reg_data_as_clk_en_2ff_sync
                                           ? w_iob_data_as_clk_en_user_sync
                                           : i_iob_data_as_clk_en_user;
    and and_icg_en_internal ( w_icg_en_internal,    
                              w_func_mode_sync,
                              w_iob_data_as_clk_en_user_muxed   
                            );
    and and_clk_internal ( w_clk_internal,    
                           w_func_mode_sync,
                           i_iob_data_as_clk_user       
                         );
    icg u_icg_block ( .i_clk     (w_clk_internal       ),
                      .i_data    (w_icg_en_internal    ),
                      .o_icg_out (w_data_as_clk_in_user)               
                    );
    always @(negedge i_all_user_clk_nrst, negedge w_clk_internal, w_icg_en_internal) begin   
            if (!i_all_user_clk_nrst) begin
                r_data_as_clk_ready_soc <= 1'b0;
            end
            else if (!w_clk_internal ) begin 
                r_data_as_clk_ready_soc <= w_icg_en_internal;
            end
    end  
    assign o_data_as_clk_in_user   = w_data_as_clk_in_user;
    assign o_data_as_clk_ready_soc = r_data_as_clk_ready_soc;
endmodule

/****** osc_ctrl\osc_ctrl.v ******/

`timescale 1ns/1ps
module osc_ctrl 
(
    input    wire    i_por_raw            ,
    input    wire    i_osc_en_request     ,
    input    wire    i_iob_osc_en         ,
    input    wire    i_func_mode          ,
    input    wire    i_reg_osc_en_2ff_sync,
    input    wire    i_all_user_clk_nrst  ,
    output   wire    o_osc_ready_user     ,
    output   wire    o_osc_ready_block    ,
    output   wire    o_soc_osc_clk        ,
    output   wire    o_user_osc_clk          
);
    wire             w_and_osc_fout_ready ;
    wire             w_iob_osc_en_sync    ;
    wire             w_iob_osc_en_sync_muxed;
    wire             w_osc_en_or_out;
    wire             w_osc_en_override;
    wire             w_func_mode_osc_sync;
    wire             w_osc_en_muxed;
    wire             w_clk_dff;
    wire             w_osc_fout;
    wire             w_internal_osc_en;
    wire             w_osc_ready_block;
    wire             w_not_and_osc_fout_ready;
    wire             w_not_func_mode;
    wire             w_dff_icg_en_soc;
    wire             w_icg_en_soc;
    wire             w_icg_en_user;
    wire             w_soc_osc_clk;
    wire             w_user_osc_clk;
    reg              r_iob_osc_ready;
    i2xDFF u_block_2xDFF_en_sync ( .i_clk     (w_and_osc_fout_ready ),
                                   .i_resetb  (i_all_user_clk_nrst),
                                   .i_set     (1'b0               ),
                                   .i_data    (i_iob_osc_en       ),
                                   .o_qout    (w_iob_osc_en_sync  )
                                 );
    assign w_iob_osc_en_sync_muxed = i_reg_osc_en_2ff_sync  
                               ? i_iob_osc_en           
                               : w_iob_osc_en_sync;     
    or or_gate_osc_en ( w_osc_en_or_out,
                        i_iob_osc_en,      
                        w_iob_osc_en_sync_muxed         
                      );
    nand nand_osc_en_override ( w_osc_en_override,    
                                i_func_mode,
                                w_func_mode_osc_sync   
                              );
    assign w_osc_en_muxed = w_osc_en_override     
                          ? i_osc_en_request      
                          : w_osc_en_or_out;      
    not not_clk ( w_clk_dff,
                  w_osc_fout
                );
    i2xDFF u_block_2xDFF ( .i_clk     (w_clk_dff        ),
                           .i_resetb  (i_por_raw        ),
                           .i_set     (w_osc_en_muxed   ),
                           .i_data    (1'b0             ),
                           .o_qout    (w_internal_osc_en)
                         );
    osc u_osc ( .i_osc_en    (w_internal_osc_en),
                .i_osc_mode  (1'b1             ),
                .o_osc_ready (w_osc_ready_block),
                .o_fout      (w_osc_fout       )
              );
    and and_osc_fout_ready ( w_and_osc_fout_ready,    
                             w_osc_fout,
                             w_osc_ready_block      
                           );
    not not_and_osc_fout_ready ( w_not_and_osc_fout_ready,
                                 w_and_osc_fout_ready
                               );
    not not_func_mode ( w_not_func_mode,
                        i_func_mode
                      );
    i2xDFF u_block_2xDFF_soc ( .i_clk     (w_not_and_osc_fout_ready),
                               .i_resetb  (i_por_raw               ),
                               .i_set     (1'b0                    ),
                               .i_data    (w_not_func_mode         ),
                               .o_qout    (w_dff_icg_en_soc        )
                             );
    i2xDFF u_block_2xDFF_user ( .i_clk     (w_not_and_osc_fout_ready),
                                .i_resetb  (i_all_user_clk_nrst     ),
                                .i_set     (1'b0                    ),
                                .i_data    (i_func_mode             ),
                                .o_qout    (w_func_mode_osc_sync    ) 
                              );
    and and_icg_en_soc ( w_icg_en_soc,    
                         i_osc_en_request,
                         w_osc_ready_block,
                         w_dff_icg_en_soc      
                       );
    and and_icg_en_user ( w_icg_en_user,    
                          w_osc_en_muxed,
                          w_osc_ready_block,
                          w_func_mode_osc_sync       
                        );
    icg u_icg_soc ( .i_clk     (w_osc_fout   ),    
                    .i_data    (w_icg_en_soc ),   
                    .o_icg_out (w_soc_osc_clk)  
                  );
    icg u_icg_user ( .i_clk     (w_osc_fout    ),    
                     .i_data    (w_icg_en_user ),   
                     .o_icg_out (w_user_osc_clk)  
                   );
    always @(negedge i_all_user_clk_nrst, negedge w_osc_fout, w_icg_en_user) begin   
            if (!i_all_user_clk_nrst) begin
                r_iob_osc_ready <= 1'b0;
            end
            else if (!w_osc_fout ) begin 
                r_iob_osc_ready <= w_icg_en_user;
            end
    end      
    assign o_osc_ready_user  = r_iob_osc_ready;
    assign o_osc_ready_block = w_osc_ready_block;
    assign o_soc_osc_clk     = w_soc_osc_clk;  
    assign o_user_osc_clk    = w_user_osc_clk;  
endmodule

/****** osc_ctrl\2xDFF\2xDFF.v ******/

`timescale 1ns/10ps
module i2xDFF 
(
    input  wire  i_clk,       
    input  wire  i_resetb,    
    input  wire  i_set,       
    input  wire  i_data,      
    output reg   o_qout       
);
    reg          r_qout_dff1 = 0; 
    always @(posedge i_clk, negedge i_resetb, posedge i_set) begin 
        if (!i_resetb) begin
            r_qout_dff1 <= 1'b0;
            o_qout      <= 1'b0;
        end
        else if (i_set) begin
            r_qout_dff1 <= 1'b1;
            o_qout      <= 1'b1;
        end
        else begin
            r_qout_dff1 <= i_data;
            o_qout      <= r_qout_dff1;
        end 
    end
endmodule

/****** osc_ctrl\icg\icg.v ******/
`timescale 1ns/1ps
module icg 
(
    input  wire  i_clk,     
    input  wire  i_data,    
    output wire  o_icg_out  
);
    reg   r_latch_out;
    always @(i_clk, i_data) begin   
            if (!i_clk ) begin 
                r_latch_out <= i_data;
            end
    end               
    assign o_icg_out = i_clk & r_latch_out;  
endmodule

/****** osc_ctrl\osc\osc.v ******/

`timescale 1ns/1ps
module osc 
(
    input  wire  i_osc_en,       
    input  wire  i_osc_mode,     
    output wire  o_osc_ready,    
    output wire  o_fout          
);
    localparam HIGH_FREQ_MODE   = 10;     
    localparam LOW_FREQ_MODE    = 146;    
    reg      r_buffered_osc_ready;
    reg      r_internal_clk;             
    integer  int_init_setting_time;      
    and                                and_1 (o_fout, r_internal_clk, o_osc_ready);       
    buf #(int_init_setting_time, 0)    buf_1 (o_osc_ready, r_buffered_osc_ready);         
    initial begin               
        int_init_setting_time = i_osc_mode ? 107e3 : 386e3;
        r_buffered_osc_ready  = 1'b0;
        r_internal_clk        = 1'b0;
    end
    always @(i_osc_mode) begin
        int_init_setting_time = i_osc_mode ? 107e3 : 386e3;
    end
    always @(*) begin    
        if (i_osc_en) begin         
            r_buffered_osc_ready  <= 1'b1;    
            if (i_osc_mode) begin
                #HIGH_FREQ_MODE r_internal_clk <= ~r_internal_clk;   
            end
            else begin 
                #LOW_FREQ_MODE r_internal_clk  <= ~r_internal_clk;   
            end
        end
        else begin                 
            r_buffered_osc_ready  <= 1'b0;
            r_internal_clk        <= 1'b0;
        end
    end
endmodule

/****** pad_ctrl\gpio_retention_ring.v ******/
`timescale 1ns/1ns
module gpio_retention_ring(
input wire   i_rst_n,
input wire   i_retention_clk,
input wire   i_retention_cfg_reg_set,
input wire   i_retention_iob_set,
input wire   i_retention_cfg_reg_rst,
input wire   i_retention_iob_rst,
input  wire  i_1x_pullup_en_q   ,
input  wire  i_2x_pullup_en_q   ,
input  wire  i_2x_buffer_en_q   , 
input  wire  i_open_drain_en_q  ,
input  wire  i_digital_output_q ,
input  wire  i_digital_oe_q , 
output wire  o_1x_pullup_en_q   ,
output wire  o_2x_pullup_en_q   ,
output wire  o_2x_buffer_en_q   , 
output wire  o_open_drain_en_q  ,
output wire  o_digital_out_en_q , 
output wire  o_digital_output_q   
);
    reg r_1x_pullup_en_ret, r_2x_pullup_en_ret, r_open_drain_en_ret, r_2x_buffer_en_ret;
    always @(posedge i_retention_cfg_reg_set, negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_1x_pullup_en_ret  <= 1'b0;
            r_2x_pullup_en_ret  <= 1'b0;
            r_open_drain_en_ret <= 1'b0;
            r_2x_buffer_en_ret  <= 1'b0;
        end
        else begin
            r_1x_pullup_en_ret  <= i_1x_pullup_en_q ;
            r_2x_pullup_en_ret  <= i_2x_pullup_en_q ;
            r_open_drain_en_ret <= i_open_drain_en_q;
            r_2x_buffer_en_ret  <= i_2x_buffer_en_q ;
        end;
    end
    reg r_bypass_cfg_reg;
    always @(posedge i_retention_clk, negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_bypass_cfg_reg <= 1'b1;
        end
        else if (i_retention_cfg_reg_set) begin
            r_bypass_cfg_reg <= 1'b0;
        end
        else if (i_retention_cfg_reg_rst) begin
            r_bypass_cfg_reg <= 1'b1;
        end;
    end
    reg r_bypass_iob_dout, r_bypass_iob_oe;
    always @(posedge i_retention_clk, negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_bypass_iob_dout <= 1'b1;
            r_bypass_iob_oe   <= 1'b1;
        end
        else if (i_retention_iob_set) begin
            r_bypass_iob_dout <= 1'b0;
            r_bypass_iob_oe   <= 1'b0;
        end
        else if (i_retention_iob_rst) begin
            r_bypass_iob_dout <= 1'b1;
            r_bypass_iob_oe   <= r_bypass_iob_dout;
        end;
    end
    reg r_digital_oe_ret, r_digital_output_ret;
    always @(posedge i_retention_iob_set, negedge i_rst_n) begin
        if (!i_rst_n) begin
            r_digital_oe_ret      <= 1'b0;
            r_digital_output_ret  <= 1'b0;
        end
        else begin
            r_digital_oe_ret      <= i_digital_oe_q;
            r_digital_output_ret  <= i_digital_output_q;
        end;
    end
    assign o_1x_pullup_en_q   = r_bypass_cfg_reg  ? i_1x_pullup_en_q   :  r_1x_pullup_en_ret; 
    assign o_2x_pullup_en_q   = r_bypass_cfg_reg  ? i_2x_pullup_en_q   :  r_2x_pullup_en_ret; 
    assign o_open_drain_en_q  = r_bypass_cfg_reg  ? i_open_drain_en_q  :  r_open_drain_en_ret; 
    assign o_2x_buffer_en_q   = r_bypass_cfg_reg  ? i_2x_buffer_en_q   :  r_2x_buffer_en_ret; 
    assign o_digital_out_en_q = r_bypass_iob_oe   ? i_digital_oe_q     :  r_digital_oe_ret; 
    assign o_digital_output_q = r_bypass_iob_dout ? i_digital_output_q :  r_digital_output_ret; 
endmodule

/****** pad_ctrl\pad_ctrl.v ******/
`timescale 1ns/1ns
module pad_ctrl #(parameter GPIO_NUMB = 19) 
(
    input wire  i_rst_n,
    input wire  i_clk,
    input wire  i_retention_cfg_reg_set,
    input wire  i_retention_iob_set,
    input wire  i_retention_cfg_reg_rst,
    input wire  i_retention_iob_rst,
    input wire  i_ctrl_gpio_fn_inp_gating,
    input wire  i_ctrl_gpio_fn_out_gating,
    input wire  i_ctrl_gpio_fn_oe_gating,
    input wire [GPIO_NUMB-1 : 0] i_gpio_digital_in,                
    input wire [GPIO_NUMB-1 : 0] i_iob_digital_out,         
    input wire [GPIO_NUMB-1 : 0] i_iob_output_enable,   
    input wire                   i_ctrl_gpio_1x_pullup_en_reg,
    input wire [GPIO_NUMB-1 : 0] i_1x_pullup_en_reg_val,
    input wire [GPIO_NUMB-1 : 0] i_1x_pullup_en_def_val,
    input wire                   i_ctrl_gpio_2x_pullup_en_reg,
    input wire [GPIO_NUMB-1 : 0] i_2x_pullup_en_reg_val,
    input wire [GPIO_NUMB-1 : 0] i_2x_pullup_en_def_val,
    input wire                   i_ctrl_gpio_2x_buffer_en_reg,
    input wire [GPIO_NUMB-1 : 0] i_2x_buffer_en_reg_val,
    input wire [GPIO_NUMB-1 : 0] i_2x_buffer_en_def_val,
    input wire                   i_ctrl_gpio_open_drain_en_reg,
    input wire [GPIO_NUMB-1 : 0] i_open_drain_en_reg_val,
    input wire [GPIO_NUMB-1 : 0] i_open_drain_en_def_val,
    output wire [GPIO_NUMB-1 : 0]  o_gpio_digital_in,  
    output wire [GPIO_NUMB-1 : 0]  o_1x_pullup_en_q,
    output wire [GPIO_NUMB-1 : 0]  o_2x_pullup_en_q,
    output wire [GPIO_NUMB-1 : 0]  o_2x_buffer_en_q,
    output wire [GPIO_NUMB-1 : 0]  o_open_drain_en_q,
    output wire [GPIO_NUMB-1 : 0]  o_digital_out_en_q,
    output wire [GPIO_NUMB-1 : 0]  o_digital_output_q       
);
    wire [GPIO_NUMB-1 : 0] w_1x_pullup_en;  
    wire [GPIO_NUMB-1 : 0] w_2x_pullup_en;  
    wire [GPIO_NUMB-1 : 0] w_2x_buffer_en;  
    wire [GPIO_NUMB-1 : 0] w_open_drain_en; 
    wire [GPIO_NUMB-1 : 0] w_digital_output;
    wire [GPIO_NUMB-1 : 0] w_digital_oe;    
    genvar i;
    generate
        for(i=0; i<GPIO_NUMB; i=i+1) begin : GPIO
            assign o_gpio_digital_in[i] = i_ctrl_gpio_fn_inp_gating & i_gpio_digital_in[i];
            assign w_digital_output[i]  = i_ctrl_gpio_fn_out_gating  & i_iob_digital_out  [i];
            assign w_digital_oe    [i]  = i_ctrl_gpio_fn_oe_gating   & i_iob_output_enable[i];
            assign w_1x_pullup_en [i]  = i_ctrl_gpio_1x_pullup_en_reg  ? i_1x_pullup_en_reg_val [i] : i_1x_pullup_en_def_val [i];
            assign w_2x_pullup_en [i]  = i_ctrl_gpio_2x_pullup_en_reg  ? i_2x_pullup_en_reg_val [i] : i_2x_pullup_en_def_val [i];
            assign w_2x_buffer_en [i]  = i_ctrl_gpio_2x_buffer_en_reg  ? i_2x_buffer_en_reg_val [i] : i_2x_buffer_en_def_val [i];
            assign w_open_drain_en[i]  = i_ctrl_gpio_open_drain_en_reg ? i_open_drain_en_reg_val[i] : i_open_drain_en_def_val[i];
            gpio_retention_ring u_gpio_retention_ring(
                .i_rst_n                  	(i_rst_n                    ),
                .i_retention_clk         	(i_clk                      ),
                .i_retention_cfg_reg_set 	(i_retention_cfg_reg_set    ),
                .i_retention_iob_set     	(i_retention_iob_set        ),
                .i_retention_cfg_reg_rst 	(i_retention_cfg_reg_rst    ),
                .i_retention_iob_rst     	(i_retention_iob_rst        ),
                .i_1x_pullup_en_q        	(w_1x_pullup_en     [i]     ),
                .i_2x_pullup_en_q        	(w_2x_pullup_en     [i]     ),
                .i_2x_buffer_en_q        	(w_2x_buffer_en     [i]     ),
                .i_open_drain_en_q       	(w_open_drain_en    [i]     ),
                .i_digital_output_q      	(w_digital_output   [i]     ),
                .i_digital_oe_q          	(w_digital_oe       [i]     ),
                .o_1x_pullup_en_q        	(o_1x_pullup_en_q   [i]     ),
                .o_2x_pullup_en_q        	(o_2x_pullup_en_q   [i]     ),
                .o_2x_buffer_en_q        	(o_2x_buffer_en_q   [i]     ),
                .o_open_drain_en_q       	(o_open_drain_en_q  [i]     ),
                .o_digital_out_en_q      	(o_digital_out_en_q [i]     ),
                .o_digital_output_q      	(o_digital_output_q [i]     ));
        end
    endgenerate
endmodule

/****** pll_ctrl\pll.v ******/
`timescale 1ns/1ps
module pll(
    input wire         i_por,
    input wire         i_pll_en,
    input wire         i_ref_clk,
    input wire  [5:0]  i_reg_refdiv,
    input wire  [11:0] i_reg_fbdiv,
    input wire  [2:0]  i_reg_posdiv1_out,
    input wire  [2:0]  i_reg_posdiv2_out,
    input wire         i_reg_bypass,
    output wire        o_lock,
    output wire        o_fout
);
    real real_pll_frequency = 0.0;
    real real_ref_clk_frequency = 0.0;
    real real_pfd_frequency = 0.0;
    reg  r_pll_frequency_change = 0;
    reg  r_incorrect_freq = 0;
    reg  r_pll_en_p = 0;
    wire pll_clk_pd;
    wire pll_out_raw;
    wire pfd_out;
    wire start_up_ctrl_clk;
    wire o_incorrect_freq;
    freq_checker ref_clk_freq_check(i_ref_clk, o_incorrect_freq);
    wire [63:0] w_ref_clk_frequency_bits;
    wire ref_clk_real_ok;
    clk_to_real_freq_conv freq_converter(i_por, i_ref_clk, w_ref_clk_frequency_bits, ref_clk_real_ok);
    always @(w_ref_clk_frequency_bits, posedge o_incorrect_freq) begin
        real_ref_clk_frequency = $bitstoreal(w_ref_clk_frequency_bits);
        if(i_por && i_pll_en && ref_clk_real_ok) begin
            if (real_ref_clk_frequency > 800.0) begin
                $display("PLL: Reference Clock Frequency is too high (5MHz < Fref_clk < 800MHz).\n",);
                r_incorrect_freq = 1;
            end else
            begin
                r_incorrect_freq = 0;
            end
        end
    end
    always @(i_por, real_ref_clk_frequency, i_reg_refdiv, i_reg_fbdiv, i_reg_posdiv1_out, i_reg_posdiv2_out, i_pll_en, ref_clk_real_ok, i_reg_bypass) begin
        if (i_por == 1 && i_pll_en == 1 && ref_clk_real_ok == 1) begin
            if (i_reg_bypass == 0) begin
                if (i_reg_refdiv == 6'b000000 || i_reg_posdiv1_out == 3'b000 || i_reg_posdiv1_out == 3'b000 || i_reg_fbdiv == 12'b000000000000) begin
                    real_pll_frequency = 10; 
                    real_pfd_frequency = 10;
                    $display("PLL: Some divider <= 0.\nProhibited combination\nPLL frequency set to 10MHz, which is the lowest PLL frequency \n",);
                end
                else begin
                    real_pfd_frequency = real_ref_clk_frequency*$itor(i_reg_fbdiv)/($itor(i_reg_refdiv));
                    real_pll_frequency = real_pfd_frequency/($itor(i_reg_posdiv1_out)*$itor(i_reg_posdiv2_out));
                end
            end
            else if (i_reg_bypass == 1) begin
                if (i_reg_refdiv != 6'b000000 && i_reg_fbdiv != 12'b000000000000) begin
                    real_pfd_frequency = real_ref_clk_frequency*$itor(i_reg_fbdiv)/($itor(i_reg_refdiv));
                    real_pll_frequency = real_ref_clk_frequency;
                end 
                else begin
                    real_pfd_frequency = real_ref_clk_frequency;
                    real_pll_frequency = real_ref_clk_frequency;
                end
            end
        end
        else begin
            if (i_por == 1 && ref_clk_real_ok == 1 && i_reg_bypass == 1) begin
                real_pfd_frequency = real_ref_clk_frequency;
                real_pll_frequency = real_ref_clk_frequency;
            end
        end
    end
    nor pll_nEN_nor_reg_gate(pll_nEn, i_reg_bypass, i_pll_en);
    or pll_clk_pd_oi_reg_gate(pll_clk_pd, pll_nEn, ~i_por, r_incorrect_freq, o_incorrect_freq);
    pll_internal_osc pll_fout_clk_gen(i_ref_clk, $realtobits(real_pll_frequency), pll_clk_pd, 7'd50, ref_clk_real_ok, pll_out_raw);
    pll_internal_osc pfb_fout_clk_gen(i_ref_clk, $realtobits(real_pfd_frequency), pll_clk_pd, 7'd50, ref_clk_real_ok, pfd_out);
    always @(real_pll_frequency) begin  
        r_pll_frequency_change = !r_pll_frequency_change;
    end
    assign w_start_up = i_por & i_pll_en;
    wire w_start_up_ctrl_clk = i_reg_bypass & i_pll_en ? i_ref_clk : pfd_out;
    assign o_lock     =  w_start_up_ready;
    pll_start_up_ctrl start_up_ctrl(o_incorrect_freq, r_pll_frequency_change, w_start_up,pll_out_raw, w_start_up_ctrl_clk, w_start_up_ready);
    assign          o_lock     =  w_start_up_ready ;
    assign          o_fout     =  i_reg_bypass ? i_ref_clk : pll_out_raw;
endmodule

/****** pll_ctrl\pll_ctrl.v ******/
`timescale 1ns/1ps
module pll_ctrl (
    input wire        i_por_raw,    
    input wire        i_pll_en_override,              
    input wire        i_iob_pll_en,                                                    
    input wire        i_func_mode,     
    input wire        i_reg_pll_en_2ff_sync,    
    input wire        i_all_user_clk_nrst,   
    input wire        i_pll_ref_clk_sel,                                               
    input wire        i_ref_clk_ext,
    input wire        i_ref_clk_osc,
    input wire [5:0]  i_reg_refdiv,
    input wire [11:0] i_reg_fbdiv,
    input wire [2:0]  i_reg_posdiv1_out,
    input wire [2:0]  i_reg_posdiv2_out,
    input wire        i_iob_bypass,
    output   wire    o_pll_lock_user,
    output   wire    pll_lock_block,
    output   wire    o_soc_pll_clk,
    output   wire    o_user_pll_clk   
);
    reg r_iob_pll_ready = 0;
    wire i_ref_clk;
    wire w_bypass_or_pll_fout_and_pll_lock;
    wire w_iob_pll_en_sync;
    wire w_iob_pll_en_sync_muxed;
    wire w_pll_pd_or_out;
    wire iob_pll_en_and_init_state;
    wire w_pll_pd_muxed;
    wire u_block_2xDFF_nSet;
    wire w_internal_pll_pd;
    wire w_pll_lock;
    wire w_pll_fout;
    wire w_and_pll_fout_ready;
    wire u_block_2xDFF_1_out;
    wire u_block_2xDFF_2_out;
    wire u_block_2xDFF_3_out;
    wire w_icg_en_soc;
    wire w_icg_en_user;
    wire bypass_or_2xDFF_2_out;
    wire w_soc_pll_clk;
    wire w_user_pll_clk;
    wire w_bypass_block;
    wire w_ref_clk_ext_and_i_all_user_clk_nrst;
    and and_i_ref_clk_ext_and_i_all_user_clk_nrst(w_ref_clk_ext_and_i_all_user_clk_nrst,i_ref_clk_ext, i_all_user_clk_nrst);
    assign w_ref_clk_pre = i_pll_ref_clk_sel ? w_ref_clk_ext_and_i_all_user_clk_nrst: i_ref_clk_osc;
    assign i_ref_clk = i_pll_en_override ? i_ref_clk_osc: w_ref_clk_pre;
    i2xDFF_pll u_block_2xDFF_en_sync ( .i_clk     (w_and_pll_fout_ready ),
                                       .i_resetb  (i_all_user_clk_nrst),
                                       .i_set     (1'b0               ),
                                       .i_data    (i_iob_pll_en       ),
                                       .o_qout    (w_iob_pll_en_sync  )
                                     );
    assign w_iob_pll_en_sync_muxed = ~i_reg_pll_en_2ff_sync  
                                     ? i_iob_pll_en         
                                     : w_iob_pll_en_sync;   
    nor or_gate_pll_en ( w_pll_pd_or_out,
                        i_iob_pll_en,      
                        w_iob_pll_en_sync_muxed         
                      );
    assign w_pll_pd_muxed = i_pll_en_override     
                          ? 1'b1                  
                          : w_pll_pd_or_out;      
    and and_u_block_2xDFF_nSet (u_block_2xDFF_nSet,w_bypass_block,~i_por_raw);
    i2xDFF_pll u_block_2xDFF_0 ( .i_clk     (~w_pll_fout           ),
                                 .i_resetb  (w_pll_pd_muxed        ),
                                 .i_set     (u_block_2xDFF_nSet    ),
                                 .i_data    (1'b0                  ),
                                 .o_qout    (w_internal_pll_pd     ) 
                               );
    assign w_bypass_block = i_pll_en_override? 1'b1 : i_iob_bypass;
    pll u_pll_core(                                                                        
        .i_por                  (i_por_raw          ),
        .i_pll_en               (~w_internal_pll_pd ),
        .i_ref_clk              (i_ref_clk          ),
        .i_reg_refdiv           (i_reg_refdiv       ),
        .i_reg_fbdiv            (i_reg_fbdiv        ),
        .i_reg_posdiv1_out      (i_reg_posdiv1_out  ),
        .i_reg_posdiv2_out      (i_reg_posdiv2_out  ),
        .i_reg_bypass           (w_bypass_block     ),
        .o_lock                 (w_pll_lock         ),
        .o_fout                 (w_pll_fout         )
    );
    and and_pll_fout_and_pll_lock (w_and_pll_fout_ready, w_pll_fout, w_pll_lock);
    or or_bypass_or_pll_fout_and_pll_lock (w_bypass_or_pll_fout_and_pll_lock, w_and_pll_fout_ready, w_bypass_block);
    i2xDFF_pll u_block_2xDFF_1   ( 
                                   .i_clk     ( ~w_and_pll_fout_ready ),
                                   .i_resetb  ( i_por_raw                          ),
                                   .i_set     ( 1'b0                               ),
                                   .i_data    ( ~i_func_mode                       ), 
                                   .o_qout    ( u_block_2xDFF_1_out                )
                                 );
    i2xDFF_pll u_block_2xDFF_2   ( 
                                   .i_clk     (w_pll_fout                          ),
                                   .i_resetb  (w_internal_pll_en                   ), 
                                   .i_set     (1'b0                                ),
                                   .i_data    (w_pll_lock                          ),
                                   .o_qout    (u_block_2xDFF_2_out                 ) 
                                 );                                     
    i2xDFF_pll u_block_2xDFF_3   ( 
                                   .i_clk     (~w_and_pll_fout_ready  ),
                                   .i_resetb  (i_all_user_clk_nrst                 ),
                                   .i_set     (1'b0                                ),
                                   .i_data    (i_func_mode                         ),
                                   .o_qout    (u_block_2xDFF_3_out                 ) 
                                 );
    and and_icg_en_soc  ( 
                          w_icg_en_soc,    
                          u_block_2xDFF_1_out,
                          i_pll_en_override,
                          u_block_2xDFF_2_out      
                        );
    or or_bypass_or_2xDFF_2_out (bypass_or_2xDFF_2_out,w_bypass_block,u_block_2xDFF_2_out);
    and and_icg_en_user ( 
                          w_icg_en_user,    
                          bypass_or_2xDFF_2_out,
                          w_iob_pll_en_sync_muxed,
                          u_block_2xDFF_3_out       
                        );
    icg_pll u_icg_soc       ( 
                              .i_clk     (w_pll_fout   ),    
                              .i_data    (w_icg_en_soc ),   
                              .o_icg_out (w_soc_pll_clk)  
                            );
    icg_pll u_icg_user      ( 
                              .i_clk     (w_pll_fout    ),    
                              .i_data    (w_icg_en_user ),   
                              .o_icg_out (w_user_pll_clk)  
                            );
    always @(negedge i_all_user_clk_nrst, negedge w_pll_fout, w_icg_en_user) begin   
            if (!i_all_user_clk_nrst) begin
                r_iob_pll_ready <= 1'b0;
            end
            else if (!w_pll_fout ) begin 
                r_iob_pll_ready <= w_icg_en_user;
            end
    end      
    assign pll_lock_block      = w_pll_lock;
    assign o_pll_lock_user     = r_iob_pll_ready; 
    assign o_soc_pll_clk       = w_soc_pll_clk;  
    assign o_user_pll_clk      = w_user_pll_clk;  
endmodule

/****** pll_ctrl\2xDFF\2xDFF_pll.v ******/

`timescale 1ns/10ps
module i2xDFF_pll 
(
    input  wire  i_clk,       
    input  wire  i_resetb,    
    input  wire  i_set,       
    input  wire  i_data,      
    output wire  o_qout       
);
    reg          r_qout_dff1 = 0; 
    reg          r_qout      = 0; 
    always @(posedge i_clk, negedge i_resetb, posedge i_set) begin 
        if (!i_resetb) begin
            r_qout_dff1 <= 1'b0;
            r_qout      <= 1'b0;
        end
        else if (i_set) begin
            r_qout_dff1 <= 1'b1;
            r_qout      <= 1'b1;
        end
        else begin
            r_qout_dff1 <= i_data;
            r_qout      <= r_qout_dff1;
        end 
    end
    assign o_qout = r_qout;
endmodule

/****** pll_ctrl\clk_gen_for_osc\clk_gen_for_osc.v ******/
`timescale 1ns/1ps
module timer_part(input wire i_ref_clk, input wire i_timer_npd, input wire [63:0] i_high_time, input wire [63:0] i_low_time, input wire [63:0] i_shift_value, output o_out);
    real high_time_real, low_time_real;
    reg r_out_temp;
    real phase_shift, phase_shift_old;
    initial begin
        r_out_temp = 1;
        high_time_real = 0.1;
        low_time_real = 0.1;
        phase_shift = 0;
        phase_shift_old = 0;
    end
    always @(i_high_time, i_low_time, i_timer_npd) begin
        if (i_timer_npd == 0) begin 
            high_time_real = 0;
            low_time_real = 0;
            phase_shift = 0;
        end
        else begin
            high_time_real = $bitstoreal(i_high_time);
            low_time_real = $bitstoreal(i_low_time);
            r_out_temp = i_ref_clk;
        end
    end
    always @(posedge i_timer_npd) begin
        phase_shift_old = phase_shift;
        phase_shift = $bitstoreal(i_shift_value);
    end
    always @(posedge i_timer_npd) begin  
        fork
        begin : wave
            while (i_timer_npd == 1 &&
                   low_time_real  > 0.0 &&
                   high_time_real > 0.0) begin
                #(low_time_real)  r_out_temp = 1;
                #(high_time_real) r_out_temp = 0;
            end
        end
        begin : stop
            @ (negedge i_timer_npd);
            disable wave;   
            r_out_temp = 0; 
        end
    join  
    end
    assign o_out = r_out_temp & i_timer_npd;
endmodule

/****** pll_ctrl\clk_gen_for_osc\clk_gen_for_osc.v ******/
`timescale 1ns/1ps
module clk_gen_for_osc(input wire i_ref_clk, input wire i_timer_npd, input wire [63:0] i_high_time, input wire [63:0] i_low_time, input wire [63:0] i_shift_value, output o_out);
    reg timer1_npd;
    reg timer2_npd;
    wire r_out_temp;
    reg vdd_ok;
    initial begin  
        timer1_npd = 0;
        timer2_npd = 0;
        vdd_ok = 1;
    end 
    always @(posedge i_timer_npd) begin
        timer1_npd = 1;
        timer2_npd = 0;
    end
    always @(negedge i_timer_npd) begin
        timer1_npd = 0;
        timer2_npd = 0;
    end
    always @(i_high_time, i_low_time) begin
        if (timer1_npd == 1) begin
            timer1_npd = 0;
            timer2_npd = 1;
        end
        else if (timer2_npd == 1) begin
            timer2_npd = 0;
            timer1_npd = 1;
        end
    end
    timer_part timer1_model(i_ref_clk, timer1_npd, i_high_time, i_low_time, i_shift_value, timer1_out);
    timer_part timer2_model(i_ref_clk, timer2_npd, i_high_time, i_low_time, i_shift_value, timer2_out);
    assign o_out = timer1_npd ? timer1_out : timer2_out;
endmodule

/****** pll_ctrl\clk_to_real_freq_conv\clk_to_real_freq_conv.v ******/

`timescale 1ns/1ps
module clk_to_real_freq_conv(input wire i_por,input wire i_clk, output [63:0] o_freq, output ref_clk_real_ok);
    realtime 
      period_start = -1, 
      period_end = -1, 
      period_start_old = -1, 
      period_end_old = -1, 
      period = 0;
    real freq_temp = 0;
    reg r_n_init_state_temp;
    integer rise_edge, edge_counter = 0;
    initial begin 
        #0.001 rise_edge = `UNKNOWN;
        r_n_init_state_temp = 0;
    end
    always @(negedge i_por) begin
        #0.001 rise_edge = `UNKNOWN;
        r_n_init_state_temp = 0;
        edge_counter = 0;
    end
    always @(posedge i_clk) begin
        if (rise_edge == `UNKNOWN) begin
            rise_edge = `RISE;
        end
        if (rise_edge == `RISE) begin
            period_start_old = period_start;
            period_start = $time;
            edge_counter = edge_counter + 1;
        end
    end
    always @(negedge i_clk) begin
        if (rise_edge == `UNKNOWN) begin
            rise_edge = `FALL;
        end
        if (rise_edge == `FALL) begin 
            period_end_old = period_end;
            period_end = $time;
            edge_counter = edge_counter + 1;
        end
    end
    always @(period_start, period_end) begin
        if (edge_counter == 2) begin
            if (rise_edge == `RISE) begin 
                period = period_start - period_start_old;
            end
            else if (rise_edge == `FALL) begin
                period =  period_end - period_end_old;
            end
            freq_temp = (1.0 / period) * 10.0e2 + 1e-12;
            edge_counter = 0;
            r_n_init_state_temp = 1;
        end 
    end
    assign o_freq = $realtobits(freq_temp);
    assign #1 ref_clk_real_ok = r_n_init_state_temp;
endmodule

/****** pll_ctrl\frequency_checker\frequency_checker.v ******/
`timescale 1ns/1ps
module freq_checker (input i_ref_clk, output o_incorrect_freq);
    time last_edge_time;
    time period;
    reg r_incorrect_freq;
    initial begin
        last_edge_time = 0;
        r_incorrect_freq = 1'b0;
    end
    always @(posedge i_ref_clk) begin
        if (last_edge_time != 0) begin
            period = $time - last_edge_time;
            if (period > 200)
                r_incorrect_freq <= 1'b1;
            else
                r_incorrect_freq <= 1'b0;
        end
        last_edge_time = $time;
    end
    assign o_incorrect_freq = r_incorrect_freq;
    always begin
        #200;  
        if (last_edge_time != 0) begin
            if (($time - last_edge_time) > 202)
                r_incorrect_freq <= 1'b1;
        end
    end
endmodule

/****** pll_ctrl\icg\icg_pll.v ******/
`timescale 1ns/1ps
module icg_pll 
(
    input  wire  i_clk,     
    input  wire  i_data,    
    output wire  o_icg_out  
);
    reg   r_latch_out = 0;
    always @(i_clk, i_data) begin   
            if (!i_clk ) begin 
                r_latch_out <= i_data;
            end
    end               
    assign o_icg_out = i_clk & r_latch_out;  
endmodule

/****** pll_ctrl\pll_internal_osc\pll_internal_osc.v ******/
`timescale 1ns/1ps
module pll_internal_osc(input wire i_ref_clk, input wire [63:0] o_freq, input wire i_pd, input wire [6:0] i_duty, input wire i_init_state, output o_osc_clk);
    real freq_real; 
    integer duty_int;
    real osc_high_time;
    real osc_low_time;
    reg r_osc_active;
    wire w_osc_clk_temp;
    initial begin
        r_osc_active <= 0;
        duty_int = i_duty;
        freq_real = $bitstoreal(o_freq);
        if (i_init_state == 1 && freq_real != 0.0) begin
            osc_high_time    = duty_int/100.0 * (1.0 / (freq_real * 1.0e6) * 1e9);
            osc_low_time     = (100.0 - duty_int) / 100.0 * (1.0 / (freq_real * 1.0e6) * 1e9);
        end
        else if (freq_real == 0.0) begin
            osc_high_time = 0;
            osc_low_time = 0;
        end
    end
    always @(i_duty, o_freq) begin
        duty_int = i_duty;
        freq_real = $bitstoreal(o_freq);
        if (i_init_state == 1 && freq_real != 0.0) begin
            osc_high_time    = duty_int/100.0 * (1.0 / (freq_real * 1.0e6) * 1e9);
            osc_low_time     = (100.0 - duty_int) / 100.0 * (1.0 / (freq_real * 1.0e6) * 1e9);
        end
        else if (freq_real == 0.0) begin
            osc_high_time = 0;
            osc_low_time = 0;
        end
    end
    always @ (i_pd, i_init_state) begin
        if (i_pd == 0 && i_init_state == 1) begin
          #(0.01) r_osc_active = 1;
        end else begin
          #(0.01) r_osc_active = 0;
        end      
    end
    wire timer_pd;
    and and_osc_pd(timer_pd, r_osc_active, i_init_state);
    clk_gen_for_osc clk_gen(i_ref_clk, timer_pd, $realtobits(osc_high_time),$realtobits(osc_low_time), 64'b0, w_osc_clk_temp);
    assign o_osc_clk = w_osc_clk_temp;
endmodule

/****** pll_ctrl\pll_start_up_ctrl\pll_start_up_ctrl.v ******/


`timescale 1ns/1ps
module pll_start_up_ctrl (input o_incorrect_freq, input r_pll_frequency_change, input i_start_up, input i_clk_pll, input i_clk, output o_ready);
    integer start_up_counter = 0;
    always @(posedge i_start_up, o_incorrect_freq) begin
        start_up_counter = 0;
    end
    always @(r_pll_frequency_change) begin
        start_up_counter = 0;
    end
    always @(negedge i_clk_pll) begin
        if(i_start_up == 0) begin
            start_up_counter = 0;
        end
    end
    always @(posedge i_clk) begin
        if (start_up_counter < 600  && i_start_up == 1) 
            start_up_counter = start_up_counter + 1;
    end
    assign o_ready = start_up_counter < 600 ? 0 : 1;
endmodule