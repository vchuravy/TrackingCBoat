using DataFrames
using JSON

Base.convert(::Type{Float64}, ::NAtype) = NaN
using Gadfly

df = readtable("out/positions.csv")

mad(X) = mad(X, median(X))
mad(X, med) = median([abs(x - med) for x in X])

median_r = median(df[:Radius])
mad_r = mad(df[:Radius])

# petri dish
js = JSON.parsefile("out/petriDish.json")
petriDish = js["Circle 0"]
pR = petriDish["radius"]
px = petriDish["x"]
py = petriDish["y"]
pDcm = 25 # cm
cboatD = 0.3 #cm
ratioCboat = median_r/(0.5*cboatD)

rInfluence = 3.5
rValid = pR - rInfluence*ratioCboat

checkValid(x, y) = radius(x, y) < rValid

df[:x] = df[:x] - px 
df[:y] = df[:y] - py

radius(x, y) = √((x)^2 + (y)^2)
#polar coordinates
df[:r] = map(radius, df[:x], df[:y])
df[:ϕ] = map((x, y) -> atan2(x,y), df[:x], df[:y])

dfOld = df
print("Recorded $(size(df, 1)) data points")
# Remove all points that are probably bogus
cond = median_r - mad_r .<= df[:Radius] .<= median_r + mad_r
df = sub(df, cond)
print(" $(size(df, 1)) of them are valid and")
# remove all points that are in the influence of the border
dfValid = sub(df, map(checkValid, df[:x], df[:y]))
println(" $(size(dfValid, 1)) are outside the influence of the boundary.")
# plot(df, x = :x, y = :y)

function v_a(df)
  vx = DataArray(Float64, size(df, 1))
  vy = DataArray(Float64, size(df, 1))
  ax = DataArray(Float64, size(df, 1))
  ay = DataArray(Float64, size(df, 1))

  vx[1] = NA
  vy[1] = NA
  ax[1] = NA
  ay[1] = NA

  for i in 2:size(df, 1)
    # check that frames directly follow each other
    if df[i-1, :Frame] == df[i, :Frame]-1 
      Δy = df[i, :y] - df[i-1, :y]
      Δx = df[i, :x] - df[i-1, :x]
      Δt = df[i, :Time] - df[i-1, :Time]

      vx[i] = Δx/Δt
      vy[i] = Δy/Δt

      Δvx = vx[i] - vx[i-1]
      Δvy = vy[i] - vy[i-1]

      ax[i] = Δvx/Δt
      ay[i] = Δvy/Δt
    else #  Discontinuity 
      vx[i] = NA
      vy[i] = NA 
      ax[i] = NA
      ay[i] = NA
    end
  end 

  r = DataFrame(t = df[:, :Time], vx = vx, vy = vy, ax = ax, ay = ay)
  r[:v] = √(r[:vx].^2 .+ r[:vy].^2)
  r[:a] = √(r[:ax].^2 .+ r[:ay].^2)
  return r
end

dfVA = v_a(df)
dfValidVA = v_a(dfValid)
writetable("out/va.csv", dfVA)
writetable("out/vaFiltered.csv", dfValidVA)

function mean_windowed_frames(df, id, c, Δ)
  r = 1:Δ:size(df, 1)
  idx = Array(Float64, length(r))
  vals = Array(Float64, length(r))
  j = 1
  for i in r
    if i+Δ < size(df, 1)
      idx[j] = mean(df[i:i+Δ, id])
      vals[j] = mean(df[i:i+Δ, c])
    else 
      idx[j] = mean(df[i:end, id])
      vals[j] = mean(df[i:end, c])
    end
    j += 1
  end
  idx, vals 
end

function mean_windowed_time(df, id, c, Δt)
  r = 0:Δt:maximum(df[id])
  idx = Array(Float64, length(r)-1)
  vals = Array(Float64, length(r)-1)

  j = 1
  for t0 in r
    if j < length(r)
      t1 = t0 + Δt
      idx[j] = t0 + 0.5Δt

      cond = t0 .<= df[id] .< t1
      subdf = sub(df, cond, c)

      vals[j] = mean(subdf[c])
    end
    j += 1
  end
  idx, vals 
end

# plot(vs, x = :t, y = :v, Geom.line)