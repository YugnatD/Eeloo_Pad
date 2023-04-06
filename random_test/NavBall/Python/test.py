import numpy as np

# # write an equivalent function for tensordot with axis [2],[1]
def tensordot(a,b):
    a, b = np.asarray(a), np.asarray(b)
    newaxes_a = [0,1,2]
    newshape_a = (16,3)
    olda = [4,4]
    newaxes_b = [1,0]
    newshape_b = (3,3)
    oldb = [3]
    at = a.transpose(newaxes_a).reshape(newshape_a)
    print("at = " + str(at))
    bt = b.transpose(newaxes_b).reshape(newshape_b)
    print("bt = " + str(bt))
    res = np.dot(at, bt)
    return res.reshape(olda + oldb)

# def tensordot2(a, b):
#     a, b = np.asarray(a), np.asarray(b)
#     newshape_a = (a.shape[0]*a.shape[1],a.shape[2])
#     olda = [a.shape[0],a.shape[1]]
#     newshape_b = (b.shape[0],b.shape[0])
#     oldb = [b.shape[1]]
#     at = a.reshape(newshape_a) # 16,3
#     bt = b.reshape(newshape_b) # 3,3
#     res = np.zeros(newshape_a)
#     for i in range(newshape_a[0]):
#         for j in range(newshape_a[1]):
#             res[i,j] = at[i,0]*bt[j,0] + at[i,1]*bt[j,1] + at[i,2]*bt[j,2]
#     return res.reshape(olda + oldb)

def tensordot2(a, b):
    a, b = np.asarray(a), np.asarray(b)
    # newshape_a = (a.shape[0]*a.shape[1],a.shape[2])
    # olda = [a.shape[0],a.shape[1]]
    # newshape_b = (b.shape[0],b.shape[0])
    # oldb = [b.shape[1]]
    # at = a.reshape(newshape_a) # 16,3
    # bt = b.reshape(newshape_b) # 3,3
    res = np.zeros(a.shape)
    # for i in range(newshape_a[0]): # 16
    #     for j in range(newshape_a[1]): # 3
    #         res[i,j] = at[i,0]*bt[j,0] + at[i,1]*bt[j,1] + at[i,2]*bt[j,2]
    for i in range (a.shape[0]): # 4
        for j in range(a.shape[1]): # 4
            for k in range(a.shape[2]): # 3
                res[i,j,k] = a[i,j,0]*b[k,0] + a[i,j,1]*b[k,1] + a[i,j,2]*b[k,2]
    return res

a = np.arange(16*3).reshape(4,4,3)
b = np.arange(9).reshape(3,3)
# print("a = " + str(a))
# print("b = " + str(b))
# print(a.shape)
# print(b.shape)
c = np.tensordot(a,b, axes=([2],[1]))
# print(c[1,1,0])
# print(c)
# print(c.shape)
c2 = tensordot2(a,b)
# print(c)
# print(c2)
# test if c and c2 are the same
np.testing.assert_array_equal(c,c2)
# print("c = " + str(c2))
# print(tensordot(a,b,axes=([2],[1])))

