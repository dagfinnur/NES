
%Placeholder for where'd you do your actual simulation or what-have-you

clear

ext = '1'

%% Load Max

% Specify the path to your CSV file
csvFilePath = strcat('ESP32_ps_max_configured_', ext, 's.CSV');

% Create import options
opts = detectImportOptions(csvFilePath);
opts.Delimiter = ',';  % Assuming the delimiter is ','

% Use readtable with the specified options
dataTable = readtable(csvFilePath, opts);

% Extract the "1st Value" column
max_readings = dataTable{:, 'x1stValue'};

%% Load Min

% Specify the path to your CSV file
csvFilePath = strcat('ESP32_ps_min_configured_', ext, 's.CSV');

% Create import options
opts = detectImportOptions(csvFilePath);
opts.Delimiter = ',';  % Assuming the delimiter is ','

% Use readtable with the specified options
dataTable = readtable(csvFilePath, opts);

% Extract the "1st Value" column
min_readings = dataTable{:, 'x1stValue'};

%% Load None

% Specify the path to your CSV file
csvFilePath = strcat('ESP32_ps_none_configured_', ext, 's.CSV');

% Create import options
opts = detectImportOptions(csvFilePath);
opts.Delimiter = ',';  % Assuming the delimiter is ','

% Use readtable with the specified options
dataTable = readtable(csvFilePath, opts);

% Extract the "1st Value" column
none_readings = dataTable{:, 'x1stValue'};

%% zero pad the front of the vectors to achieve 10 leading zeros
num_zeros = 10;
max_readings = zero_pad(max_readings', num_zeros);
min_readings = zero_pad(min_readings', num_zeros);
none_readings = zero_pad(none_readings', num_zeros);

%% Select shortest time
times = sort([length(max_readings), length(min_readings), length(none_readings)])
time_len = times(1);



%% Trim readings
max_readings = max_readings(1:time_len);
min_readings = min_readings(1:time_len);
none_readings = none_readings(1:time_len);


timescale = 1/5;

Time = [0:timescale:(time_len-1)*timescale]; %Let's pretend  this is hours elapsed since midnight
%Time = Time(1:length(readings));


filename = ['readings', '.mat']; %Descriptive name timestamp

save(filename);

function [sig] = zero_pad(signal, zs)
sig = signal;
i = 1;
current_zeros = 0;
while signal(i) == 0
    current_zeros = current_zeros + 1;
    i = i +1;
end
if current_zeros == zs
    sig = sig;
elseif current_zeros > zs
    sig = sig(current_zeros-zs:end)
else
    sig = [zeros(1, (zs-current_zeros)) sig];
end
end