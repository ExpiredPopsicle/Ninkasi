-- This is just a simple test of a Lua nested loop so we can do some
-- performance comparisons. Expect this to become more elaborate in
-- the future.

count = 0
for x=1,1000 do
    for y=1,1000 do
        count = count + 1
    end
end

print(count);
