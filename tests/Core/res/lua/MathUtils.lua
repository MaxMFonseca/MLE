local MathUtils = {}

function MathUtils.sum(a, b)
	return a + b
end

function MathUtils.vec2_add(v1, v2)
	return { v1[1] + v2[1], v1[2] + v2[2] }
end

function MathUtils.is_even(n)
	return n % 2 == 0
end

return MathUtils
