%Dependencies:

%export_fig from https://uk.mathworks.com/matlabcentral/fileexchange/23629-export_fig
%This package is absolutely essential as it allows high res export of MATLAB figures

%First, load up the data we've generated

clear
load('readings.mat')

set(0,'DefaultAxesFontSize',6); %Eight point Times is suitable typeface for an IEEE paper. Same as figure caption size
set(0,'DefaultFigureColor','w')
set(0,'defaulttextinterpreter','tex') %Allows us to use LaTeX maths notation
set(0, 'DefaultAxesFontName', 'times');
    
f = figure  %Let's make a simple time series plot of notional data
set(gcf, 'Units','centimeters')

%Set figure total dimension
set(gcf, 'Position',[0 0 8.89 4]) %Absolute print dimensions of figure. 8.89cm is essential here as it is the linewidth of a column in IEEE format
%Height can be adusted as suits, but try and be consistent amongst figures for neatness
%[pos_from_left, pos_from_bottom, fig_width, fig_height]


hold on
plot(Time, (max_readings), 'LineWidth', 0.5, 'Color', [0, 0, 255]/255); %Plot as paired data, so we're explicity stipulating the time index
plot(Time, (min_readings), 'LineWidth', 0.5, 'Color', [255,0,0]/255); %Plot as paired data, so we're explicity stipulating the time index
plot(Time, (none_readings), 'LineWidth', 0.5, 'Color', [0,255,0]/255); %Plot as paired data, so we're explicity stipulating the time index
%Use a stairstep plot so as not to linearly interpolate 
hold off

axis([0 65 0 .200])

legend('MAX', 'MIN', 'NONE')

set(gca,'YTick',[0 0.030 0.080 0.2]) %Now impose sensible tickmark locations
%Let's pretend that the undervoltage limit is 19.2 kV, and the overvoltage is 21.25 kV

set(gca,'YTickLAbel',{'0', '35', '80', '200'}) %Now put in informative labels at these tickmarks

%Now sort out the horizontal axes: it needs to be shown in wallclock units

set(gca,'XTick',[0, 10, 20, 30, 40, 50, 60]) %Now impose sensible tickmark locations

%set(gca,'XTickLAbel',{'0', '0', '16:00','00:00', '08:00', '16:00', '00:00'}) %consistent tick interval


%Set size and position of axes plotting area within figure dimensions
%It is nice to keep the vertical axes aligned for multiple figures, so be consistent with the horizontal positioning of axes 
set(gca, 'Units','centimeters')
set(gca, 'Position',[2 0.8 6.8 2.9]) %This is the relative positioning of the axes within the frame. 
%[inset_from_left, inset_from_bottom, axes_width, axes_height]

box off %Removes the borders of the plot area

ylabel({'mA'}) %Note cell matrices for line breaks
    
set(get(gca,'YLabel'),'Rotation',0, 'VerticalAlignment','middle', 'HorizontalAlignment','right') %Tidy it with right orientation (If all our vertical axes have the same internal offset all our axis labels will be neatly aligned


xlabel('seconds') %Note use of unicode arrow for clarity

%Now ready for export

filename = ['1s_testing', '.png'] %Descriptive name timestamp and .png file format

exportgraphics(f, filename, 'Resolution',300)
close(gcf)
