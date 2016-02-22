module Flutter

function dist(x)
    [sqrt(x[i,1]*x[i,1]+x[i,2]*x[i,2]+1e4*x[i,3]*x[i,3]) for i=1:size(x,1)]
end

function read(path)
    readdlm(path,'\t';header=true)
end

function readdist(path)
    x,h=read(path)
    [x[:,1] dist(x[:,2:4]) dist(x[:,5:7]) dist(x[:,8:10])],h
end

function diff(x)
    [x[2:end,1] diff(x[:,2:end])]
end

end
